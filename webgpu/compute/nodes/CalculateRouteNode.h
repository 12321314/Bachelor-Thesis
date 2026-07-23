#pragma once

#include "Node.h"
#include "webgpu/base/Context.h"

namespace webgpu_compute::nodes {

class CalculateRouteNode : public Node
{
    Q_OBJECT

public:
    NODE_TYPE_NAME(CalculateRouteNode)

    struct CalculateRouteNodeSettings
    {
        WGPUTextureFormat format = WGPUTextureFormat_RGBA8Unorm;
        WGPUTextureUsage usage =
            WGPUTextureUsage_TextureBinding |
            WGPUTextureUsage_CopyDst;
    };

    explicit CalculateRouteNode(webgpu::Context& ctx);
    CalculateRouteNode(webgpu::Context& ctx, const CalculateRouteNodeSettings& settings);

    void calculateRoute(double lat1, double lon1, double lat2, double lon2);
    float movementCost(CostCell& current, CostCell& neighbor, bool diagonal);

    void set_settings(const CalculateRouteNodeSettings& settings);
    const CalculateRouteNodeSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

    void check_progress_and_emit_signals();

    std::unique_ptr<webgpu::raii::TextureWithSampler> m_texture;
    CostGrid m_route_grid; // the route path embedded into a cost grid
    std::vector<glm::vec4> m_path; // the route path transformed to world coordinates as a vec4 so it can be directly used by the track renderer

    struct CompareCells {
        bool operator()(CostCell* a, CostCell* b) {
            return a->fCost > b->fCost;
        }
    };

public slots:
    void run_impl() override;

private:
    webgpu::Context* m_ctx;
    CalculateRouteNodeSettings m_settings;
};

} // namespace webgpu_compute::nodes