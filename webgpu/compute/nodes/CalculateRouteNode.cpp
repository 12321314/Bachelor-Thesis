#include "CalculateRouteNode.h"

#include "nucleus/srs.h"
#include "webgpu/engine/track/TrackRenderer.h"

namespace webgpu_compute::nodes {


CalculateRouteNode::CalculateRouteNode(webgpu::Context& ctx)
    : CalculateRouteNode(ctx, CalculateRouteNodeSettings {})
{
}

CalculateRouteNode::CalculateRouteNode(webgpu::Context& ctx,const CalculateRouteNodeSettings& settings)
: Node(
      {
          InputSocket(*this, "cost grid", data_type<const CostGrid*>()),
      },
      {
          OutputSocket(*this, "texture", data_type<const webgpu::raii::TextureWithSampler*>(), [this]() { return m_texture.get();}),
          OutputSocket(*this, "route", data_type<const RouteInfo*>(), [this]() { return &m_route_info;}),
      })
      , m_ctx(&ctx)
      , m_settings(settings)
{
    set_settings(settings);
}

void CalculateRouteNode::run_impl()
{
    const auto cost_grid = std::get<data_type<const CostGrid*>()>(input_socket("cost grid").get_connected_data());
    std::vector<std::pair<double, double>> points = {
        {47.778737, 15.824605},
        {47.776979, 15.814548},
        {47.774417, 15.809530}
    };
    m_route_grid.width = cost_grid->width;
    m_route_grid.height = cost_grid->height;
    m_route_grid.cell_size_meters = cost_grid->cell_size_meters;
    m_route_grid.origin_x = cost_grid->origin_x;
    m_route_grid.origin_y = cost_grid->origin_y;
    m_route_grid.costs = cost_grid->costs;

    // -----------------------------
    // Calculate the Route
    // -----------------------------

    m_route_info.path.clear();
    m_route_info.colors.clear();
    m_route_info.points.clear();

    for (int i = 0; i < points.size(); i++) {
        // set control points
        glm::dvec2 lat_lon1 = glm::dvec2(points[i].first, points[i].second);
        lat_lon1 = nucleus::srs::lat_long_to_world(lat_lon1);

        int controlX = (int)((lat_lon1.x - m_route_grid.origin_x) / m_route_grid.cell_size_meters);
        int controlY = (int)((m_route_grid.origin_y - lat_lon1.y) / m_route_grid.cell_size_meters);
        float controlHeight = m_route_grid.costs[controlY * m_route_grid.width + controlX].elevation;

        m_route_info.points.emplace_back(lat_lon1.x, lat_lon1.y, controlHeight * 1.49, 1.0f);

        // route planning
        if (i + 1 < points.size()) {
            calculateRoute(points[i].first, points[i].second, points[i + 1].first, points[i + 1].second);
            qInfo() << points[i].first << " from to " << points[i + 1].first;
        }
    }

    // -----------------------------
    // Create Output Texture
    // -----------------------------

    WGPUTextureDescriptor texture_desc{};
    texture_desc.label = WGPUStringView{ .data = "route texture", .length = WGPU_STRLEN };
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.size = { m_route_grid.width, m_route_grid.height, 1 };
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.format = WGPUTextureFormat_RGBA8Unorm;
    texture_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

    WGPUSamplerDescriptor sampler_desc{};
    sampler_desc.label = WGPUStringView{ .data = "route sampler", .length = WGPU_STRLEN };
    sampler_desc.addressModeU = WGPUAddressMode_ClampToEdge;
    sampler_desc.addressModeV = WGPUAddressMode_ClampToEdge;
    sampler_desc.addressModeW = WGPUAddressMode_ClampToEdge;
    sampler_desc.magFilter = WGPUFilterMode_Linear;
    sampler_desc.minFilter = WGPUFilterMode_Linear;
    sampler_desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
    sampler_desc.lodMinClamp = 0.0f;
    sampler_desc.lodMaxClamp = 1.0f;
    sampler_desc.maxAnisotropy = 1;

    m_texture = std::make_unique<webgpu::raii::TextureWithSampler>(
        m_ctx->device(),
        texture_desc,
        sampler_desc);

    // -----------------------------
    // Convert Route Grid to RGBA
    // -----------------------------

    std::vector<glm::u8vec4> pixels( m_route_grid.width *  m_route_grid.height);


    for (size_t i = 0; i < pixels.size(); i++)
    {
        float selected = m_route_grid.costs[i].selected;
        float control_point = m_route_grid.costs[i].controlPoint;

        if (control_point) {

            pixels[i] = glm::u8vec4(
             0,
             0,
             255,
             255);

        } else if (selected) {

            pixels[i] = glm::u8vec4(
             255,
             0,
             0,
             255);

        } else {

            pixels[i] = glm::u8vec4(
             0,
             0,
             0,
             0);

        }
    }

    // -----------------------------
    // Upload to GPU
    // -----------------------------

    WGPUTexelCopyTextureInfo image_copy_texture{};
    image_copy_texture.texture = m_texture->texture().handle();
    image_copy_texture.aspect = WGPUTextureAspect_All;
    image_copy_texture.mipLevel = 0;
    image_copy_texture.origin = {0, 0, 0};

    WGPUTexelCopyBufferLayout layout{};
    layout.offset = 0;
    layout.bytesPerRow = uint32_t(sizeof(glm::u8vec4) * m_route_grid.width);
    layout.rowsPerImage = m_route_grid.height;

    WGPUExtent3D extent{};
    extent.width = m_route_grid.width;
    extent.height = m_route_grid.height;
    extent.depthOrArrayLayers = 1;

    wgpuQueueWriteTexture(
        m_ctx->queue(),
        &image_copy_texture,
        pixels.data(),
        uint32_t(pixels.size() * sizeof(glm::u8vec4)),
        &layout,
        &extent);

    complete_run();
}

void CalculateRouteNode::calculateRoute(double lat1, double lon1, double lat2, double lon2)
{
    for (auto& cell : m_route_grid.costs) {
        cell.visited = false;
        cell.gCost = std::numeric_limits<float>::max();
        cell.hCost = 0.0f;
        cell.fCost = std::numeric_limits<float>::max();
        cell.parentX = -1;
        cell.parentY = -1;
    }

    glm::dvec2 lat_lon1 = glm::dvec2(lat1, lon1);
    glm::dvec2 lat_lon2 = glm::dvec2(lat2, lon2);
    lat_lon1 = nucleus::srs::lat_long_to_world(lat_lon1);
    lat_lon2 = nucleus::srs::lat_long_to_world(lat_lon2);

    int startX = (int)((lat_lon1.x - m_route_grid.origin_x) / m_route_grid.cell_size_meters);
    int startY = (int)((m_route_grid.origin_y - lat_lon1.y) / m_route_grid.cell_size_meters);
    int endX = (int)((lat_lon2.x - m_route_grid.origin_x) / m_route_grid.cell_size_meters);
    int endY = (int)((m_route_grid.origin_y - lat_lon2.y) / m_route_grid.cell_size_meters);

    //mark control points
    m_route_grid.costs[startY * m_route_grid.width + startX].controlPoint = true;
    m_route_grid.costs[endY * m_route_grid.width + endX].controlPoint = true;

    CostCell* start = &m_route_grid.costs[startY * m_route_grid.width + startX];

    start->gCost = 0;
    start->fCost = 0;

    std::priority_queue<CostCell*,std::vector<CostCell*>,CompareCells> openSet;

    openSet.push(start);

    while (!openSet.empty()) {

        CostCell* current = openSet.top();
        openSet.pop();

        CostCell* currentCell = &m_route_grid.costs[current->y * m_route_grid.width + current->x];

        if (currentCell->cost == 99) {
            // cannot access this cell
            currentCell->visited = true;
            continue;
        }

        currentCell->visited = true;

        // if end reached
        if (current->x == endX && current->y == endY) {
            break;
        }

        // go through all neighbors
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {

                if (dx == 0 && dy == 0)
                    continue;

                int nx = current->x + dx;
                int ny = current->y + dy;

                if (nx < 0 || ny < 0 ||
                    nx >= m_route_grid.width || ny >= m_route_grid.height)
                    continue;

                CostCell* neighbor = &m_route_grid.costs[ny * m_route_grid.width + nx];

                if (neighbor->visited)
                    continue;

                bool diagonal =
                    (dx != 0 && dy != 0);

                float moveCost =
                    movementCost(
                        *currentCell,
                        *neighbor,
                        diagonal
                    );

                float tentativeG =
                    currentCell->gCost +
                    moveCost;

                if (tentativeG < neighbor->gCost) {

                    neighbor->gCost = tentativeG;

                    float hx = endX - nx;
                    float hy = endY - ny;

                    neighbor->hCost = sqrt(hx * hx + hy * hy);

                    neighbor->fCost = neighbor->gCost + neighbor->hCost;
                    neighbor->parentX = current->x;
                    neighbor->parentY = current->y;

                    openSet.push(neighbor);
                }
            }
        }
    }

    // reconstructing path from end to start

    int cx = endX;
    int cy = endY;

    std::vector<glm::vec4> segment;

    while (!(cx == startX && cy == startY)) {

        float worldX = m_route_grid.origin_x + (cx + 0.5) * m_route_grid.cell_size_meters;
        float worldY = m_route_grid.origin_y - (cy + 0.5) * m_route_grid.cell_size_meters;
        float worldZ = m_route_grid.costs[cy * m_route_grid.width + cx].elevation;
        segment.emplace_back(worldX, worldY, worldZ * 1.49, 1.0f); //TODO figure out how the z coords work

        convertCostToColor(m_route_grid.costs[cy * m_route_grid.width + cx].cost);

        m_route_grid.costs[cy * m_route_grid.width + cx].selected = true;

        CostCell& c = m_route_grid.costs[cy * m_route_grid.width + cx];

        int px = c.parentX;
        int py = c.parentY;

        if (px == -1 || py == -1)
            break;

        cx = px;
        cy = py;
    }

    std::reverse(segment.begin(), segment.end());

    m_route_info.path.insert(
    m_route_info.path.end(),
    segment.begin(),
    segment.end()
    );
}

void CalculateRouteNode::convertCostToColor(float cost)
{
    float t = cost / 99.0f;

    float r, g, b, a;

    a = 255;

    if (t < 0.5f) {

        // green to yellow
        float k = t / 0.5f;

        r = (255 * k);
        g = 255;
        b = 0;

    } else {

        // yellow to red
        float k = (t - 0.5f) / 0.5f;

        r = 255;
        g = (255 * (1.0f - k));
        b = 0;
    }

    // special case
    if (cost == 99) {
        r = 0;
        g = 0;
        b = 0;
    }

    m_route_info.colors.emplace_back(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

float CalculateRouteNode::movementCost(CostCell& current, CostCell& neighbor, bool diagonal)
{

    float distance = diagonal ? 14.142f : 10.0f;

    float dh = neighbor.elevation - current.elevation;

    float slope = dh / distance;

    float velocity = 6.0f * exp(-3.5f * fabs(slope + 0.05f)); // hiking function

    float cost = distance / velocity;

    cost *= neighbor.cost * 0.1f; // terrain modifiers

    return cost;
}

void CalculateRouteNode::set_settings(const CalculateRouteNodeSettings& settings)
{
    m_settings = settings;
}

void CalculateRouteNode::check_progress_and_emit_signals()
{
    complete_run();
}

void CalculateRouteNode::serialize_settings(QJsonObject& out) const
{
}

void CalculateRouteNode::deserialize_settings(const QJsonObject& in)
{
}

} // namespace webgpu_compute::nodes