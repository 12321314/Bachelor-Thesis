#pragma once

#include "Node.h"
#include "webgpu/base/Context.h"

namespace webgpu_compute::nodes {

class CostSurfaceTextureNode : public Node
{
    Q_OBJECT

public:
    NODE_TYPE_NAME(CreateCostSurfaceTextureNode)

    struct CostSurfaceTextureNodeSettings
    {
        std::string overlay_mode = "Cost"; // can be Cost, Paths, or Cliffs

        WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm;
        WGPUTextureUsage usage =
            WGPUTextureUsage_TextureBinding |
            WGPUTextureUsage_CopyDst;
    };

    explicit CostSurfaceTextureNode(webgpu::Context& ctx);
    CostSurfaceTextureNode(webgpu::Context& ctx, const CostSurfaceTextureNodeSettings& settings);

    void set_settings(const CostSurfaceTextureNodeSettings& settings);
    const CostSurfaceTextureNodeSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

    void check_progress_and_emit_signals();

    std::unique_ptr<webgpu::raii::TextureWithSampler> m_texture;

public slots:
    void run_impl() override;

private:
    webgpu::Context* m_ctx;
    CostSurfaceTextureNodeSettings m_settings;
};

} // namespace webgpu_compute::nodes