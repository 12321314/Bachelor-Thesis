#include "CostSurfaceTextureNode.h"

#include "CreateHeightGrid.h"
#include "util.h"
#include <QColor>
#include <QImage>

#include <QDebug>
#include <QFileInfo>
#include <set>

namespace webgpu_compute::nodes {


CostSurfaceTextureNode::CostSurfaceTextureNode(webgpu::Context& ctx)
    : CostSurfaceTextureNode(ctx, CostSurfaceTextureNodeSettings {})
{
}

CostSurfaceTextureNode::CostSurfaceTextureNode(webgpu::Context& ctx,const CostSurfaceTextureNodeSettings& settings)
: Node(
      {
          InputSocket(*this, "cost grid", data_type<const CostGrid*>()),
      },
      {
          OutputSocket(*this, "texture", data_type<const webgpu::raii::TextureWithSampler*>(), [this]() { return m_texture.get();}),
      })
      , m_ctx(&ctx)
      , m_settings(settings)
{
    set_settings(settings);
}


void CostSurfaceTextureNode::run_impl()
{
    const auto grid = std::get<data_type<const CostGrid*>()>(input_socket("cost grid").get_connected_data());

    // -----------------------------
    // Create output texture
    // -----------------------------

    WGPUTextureDescriptor texture_desc{};
    texture_desc.label = WGPUStringView{ .data = "cost surface texture", .length = WGPU_STRLEN };
    texture_desc.dimension = WGPUTextureDimension_2D;
    texture_desc.size = { grid->width, grid->height, 1 };
    texture_desc.mipLevelCount = 1;
    texture_desc.sampleCount = 1;
    texture_desc.format = WGPUTextureFormat_RGBA8Unorm;
    texture_desc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

    WGPUSamplerDescriptor sampler_desc{};
    sampler_desc.label = WGPUStringView{ .data = "cost surface sampler", .length = WGPU_STRLEN };
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
    // Convert cost grid to RGBA
    // -----------------------------

    std::vector<glm::u8vec4> pixels(grid->width * grid->height);

    if (m_settings.overlay_mode == "Cost")
    {
        for (size_t y = 0; y < grid->height; y++)
        {
            for (size_t x = 0; x < grid->width; x++)
            {
                float cost = grid->costs[y * grid->width + x].cost;
                float t = cost / 99.0f;

                unsigned char r, g, b, a;

                a = 255;

                if (t < 0.5f) {

                    // green to yellow
                    float k = t / 0.5f;

                    r = (unsigned char)(255 * k);
                    g = 255;
                    b = 0;

                } else {

                    // yellow to red
                    float k = (t - 0.5f) / 0.5f;

                    r = 255;
                    g = (unsigned char)(255 * (1.0f - k));
                    b = 0;
                }

                // special case
                if (cost == 99) {
                    r = 0;
                    g = 0;
                    b = 0;
                }

                // this makes all cost 99 borders disappear
                if (y == 0 || y == grid->height - 1 || x == 0 || x == grid->width - 1) {
                    r = 0;
                    g = 0;
                    b = 0;
                    a = 0;
                }

                pixels[y * grid->width + x] = glm::u8vec4(
                    r,
                    g,
                    b,
                    a);
            }
        }
    }
    else if (m_settings.overlay_mode == "Paths")
    {
        for (size_t i = 0; i < pixels.size(); i++)
        {
            bool path = grid->costs[i].path;

            if (path) {
                pixels[i] = glm::u8vec4(
                255,
                255,
                0,
                255);
            }
            else {
                pixels[i] = glm::u8vec4(
                0,
                0,
                0,
                0);
            }
        }
    }
    else if (m_settings.overlay_mode == "Cliffs")
    {
        for (size_t i = 0; i < pixels.size(); i++)
        {
            float cost = grid->costs[i].cost;

            if (cost == 99) {
                pixels[i] = glm::u8vec4(
                255,
                0,
                0,
                255);
            }
            else {
                pixels[i] = glm::u8vec4(
                0,
                0,
                0,
                0);
            }
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
    layout.bytesPerRow = uint32_t(sizeof(glm::u8vec4) * grid->width);
    layout.rowsPerImage = grid->height;

    WGPUExtent3D extent{};
    extent.width = grid->width;
    extent.height = grid->height;
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

void CostSurfaceTextureNode::set_settings(const CostSurfaceTextureNodeSettings& settings)
{
    m_settings = settings;
}

void CostSurfaceTextureNode::check_progress_and_emit_signals()
{
    complete_run();
}

void CostSurfaceTextureNode::serialize_settings(QJsonObject& out) const
{
}

void CostSurfaceTextureNode::deserialize_settings(const QJsonObject& in)
{
}

} // namespace webgpu_compute::nodes