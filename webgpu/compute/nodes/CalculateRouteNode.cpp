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
          InputSocket(*this, "points", data_type<const nucleus::track::Segment*>()),
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
    const auto points = std::get<data_type<const nucleus::track::Segment*>()>(input_socket("points").get_connected_data());
    m_route_grid.width = cost_grid->width;
    m_route_grid.height = cost_grid->height;
    m_route_grid.cell_size_meters = cost_grid->cell_size_meters;
    m_route_grid.origin_x = cost_grid->origin_x;
    m_route_grid.origin_y = cost_grid->origin_y;
    m_route_grid.costs = cost_grid->costs;

    // -----------------------------
    // Calculate the Route
    // -----------------------------

    m_route_info.path_one.clear();
    m_route_info.path_two.clear();
    m_route_info.path_three.clear();
    m_route_info.colors_one.clear();
    m_route_info.colors_two.clear();
    m_route_info.colors_three.clear();
    m_route_info.points.clear();
    m_route_number = 0;

    // set control points
    for (int i = 0; i < points->size(); i++) {
        glm::dvec2 lat_lon1 = glm::dvec2(points->at(i).latitude, points->at(i).longitude);
        lat_lon1 = nucleus::srs::lat_long_to_world(lat_lon1);

        int controlX = (int)((lat_lon1.x - m_route_grid.origin_x) / m_route_grid.cell_size_meters);
        int controlY = (int)((m_route_grid.origin_y - lat_lon1.y) / m_route_grid.cell_size_meters);
        float controlHeight = m_route_grid.costs[controlY * m_route_grid.width + controlX].elevation;

        m_route_info.points.emplace_back(lat_lon1.x, lat_lon1.y, controlHeight * (1 / cos(points->at(i).latitude * pi / 180)), 1.0f);
    }

    // route planning
    while (m_route_number < m_settings.number_of_routes) {
        for (int i = 0; i < points->size(); i++) {

            if (i + 1 < points->size()) {
                calculateRoute(points->at(i).latitude, points->at(i).longitude, points->at(i + 1).latitude, points->at(i + 1).longitude);
                qInfo() << points->at(i).latitude << " from to " << points->at(i).longitude;
                qInfo() << "route: " << m_route_number;
            }
        }
        m_route_number++;
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
        float selected_one = m_route_grid.costs[i].selected_one;
        float selected_two = m_route_grid.costs[i].selected_two;
        float selected_three = m_route_grid.costs[i].selected_three;
        float control_point = m_route_grid.costs[i].controlPoint;
        glm::vec4 color = convertCostToColor(m_route_grid.costs[i].cost);

        if (control_point) {

            pixels[i] = glm::u8vec4(
             0,
             0,
             255,
             255);

        } else if (selected_one) {

            pixels[i] = glm::u8vec4(
             color.x,
             color.y,
             color.z,
             255);

        } else if (selected_two) {

            pixels[i] = glm::u8vec4(
             color.x,
             color.y,
             color.z,
             100);

        } else if (selected_three) {

            pixels[i] = glm::u8vec4(
             color.x,
             color.y,
             color.z,
             100);

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

        //apply route penalty
        if ((m_route_number == 1 && cell.selected_one) || (m_route_number == 2 && cell.selected_two)) {
            cell.simulated_cost = cell.simulated_cost * 1.5;
            if (cell.simulated_cost > 99) {
                cell.simulated_cost = 99;
            }
        }
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

        if (currentCell->simulated_cost == 99) {
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

        glm::dvec2 lat_lon = glm::dvec2(worldX, worldY);
        lat_lon = nucleus::srs::world_to_lat_long(lat_lon);

        segment.emplace_back(worldX, worldY, worldZ * (1 / cos(lat_lon.x * pi / 180)), 1.0f);

        glm::vec4 color = convertCostToColor(m_route_grid.costs[cy * m_route_grid.width + cx].cost);

        if (m_route_number == 0) {
            m_route_grid.costs[cy * m_route_grid.width + cx].selected_one = true;
            m_route_info.colors_one.emplace_back(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, color.w / 255.0f);
        }
        if (m_route_number == 1) {
            m_route_grid.costs[cy * m_route_grid.width + cx].selected_two = true;
            m_route_info.colors_two.emplace_back(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, color.w / 255.0f);
        }
        if (m_route_number == 2) {
            m_route_grid.costs[cy * m_route_grid.width + cx].selected_three = true;
            m_route_info.colors_three.emplace_back(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f, color.w / 255.0f);
        }

        CostCell& c = m_route_grid.costs[cy * m_route_grid.width + cx];

        int px = c.parentX;
        int py = c.parentY;

        if (px == -1 || py == -1)
            break;

        cx = px;
        cy = py;
    }

    std::reverse(segment.begin(), segment.end());

    if (m_route_number == 0) {
        m_route_info.path_one.insert(m_route_info.path_one.end(), segment.begin(), segment.end());
    }
    if (m_route_number == 1) {
        m_route_info.path_two.insert(m_route_info.path_two.end(), segment.begin(), segment.end());
    }
    if (m_route_number == 2) {
        m_route_info.path_three.insert(m_route_info.path_three.end(), segment.begin(), segment.end());
    }
}

glm::vec4 CalculateRouteNode::convertCostToColor(float cost)
{
    float t = cost / 99.0f;

    float r, g, b, a;

    a = 255;

    if (t < 0.3f) {

        // green to yellow
        float k = t / 0.3f;

        r = (255 * k);
        g = 255;
        b = 0;

    } else {

        // yellow to red
        float k = (t - 0.3f) / 0.7f;

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

    glm::vec4 color = glm::vec4(r, g, b, a);

    return color;
}

float CalculateRouteNode::movementCost(CostCell& current, CostCell& neighbor, bool diagonal)
{

    float distance = diagonal ? 14.142f : 10.0f;

    float dh = neighbor.elevation - current.elevation;

    float slope = dh / distance;

    float velocity = 6.0f * exp(-3.5f * fabs(slope + 0.05f)); // hiking function

    float cost = distance / velocity;

    cost *= neighbor.simulated_cost * 0.5f; // terrain modifiers

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