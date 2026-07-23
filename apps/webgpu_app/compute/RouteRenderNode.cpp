#include "RouteRenderNode.h"

#include "nucleus/srs.h"
#include <QDebug>
#include <webgpu/webgpu.h>
#include <webgpu/engine/Context.h>
#include <webgpu/engine/overlay/OverlayRenderer.h>
#include <webgpu/engine/overlay/TextureOverlay.h>

namespace webgpu_compute::nodes {

RouteRenderNode::RouteRenderNode(webgpu_engine::Context& context)
    : RouteRenderNode(context, RouteRenderNodeSettings {})
{
}

RouteRenderNode::RouteRenderNode(webgpu_engine::Context& context, const RouteRenderNodeSettings& settings)
    : Node({
        InputSocket(*this, "route", data_type<const std::vector<glm::vec4>>() ),
                         },
          {})
    , m_context(&context)
    , m_settings(settings)
{
}

RouteRenderNode::~RouteRenderNode()
{

}

void RouteRenderNode::run_impl()
{

    std::vector<glm::vec4> route = std::get<data_type<const std::vector<glm::vec4>>()>(input_socket("route").get_connected_data());

    glm::vec4 color = {255, 0, 0, 1};

    m_context->track_renderer()->add_world_positions(route, color);
    m_context->request_redraw();

    complete_run();
}

} // namespace webgpu_compute::nodes
