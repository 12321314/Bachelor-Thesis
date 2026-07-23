#pragma once

#include <memory>
#include <radix/geometry.h>
#include <webgpu/base/raii/TextureWithSampler.h>
#include <webgpu/compute/nodes/Node.h>

namespace webgpu_engine {
class Context;
} // namespace webgpu_engine

namespace webgpu_compute::nodes {

// A custom node that forwards a calculated route to the TrackRenderer.
// Unlike a base compute node it knows the rendering layer.
class RouteRenderNode : public Node {
    Q_OBJECT

public:
    NODE_TYPE_NAME(RouteRenderNode)

    struct RouteRenderNodeSettings {
        // false: link the source texture directly (non-owning).
        // true: copy the source into the overlay's own texture (requires CopySrc on the source).
        bool copy = false;
    };

    explicit RouteRenderNode(webgpu_engine::Context& context);
    RouteRenderNode(webgpu_engine::Context& context, const RouteRenderNodeSettings& settings);
    ~RouteRenderNode() override;

    void set_settings(const RouteRenderNodeSettings& settings) { m_settings = settings; }
    const RouteRenderNodeSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override {}
    void deserialize_settings(const QJsonObject& in) override {}

public slots:
    void run_impl() override;

private:
    webgpu_engine::Context* m_context;
    RouteRenderNodeSettings m_settings;
};

} // namespace webgpu_compute::nodes