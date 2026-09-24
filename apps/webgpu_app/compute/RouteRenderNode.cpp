#include "RouteRenderNode.h"

#include "nucleus/srs.h"
#include "webgpu/compute/types/RouteInfo.h"

#include <QDebug>
#include <webgpu/engine/Context.h>
#include <webgpu/engine/overlay/OverlayRenderer.h>
#include <webgpu/engine/overlay/TextureOverlay.h>
#include <webgpu/webgpu.h>

namespace webgpu_compute::nodes {

RouteRenderNode::RouteRenderNode(webgpu_engine::Context& context)
    : RouteRenderNode(context, RouteRenderNodeSettings {})
{
}

RouteRenderNode::RouteRenderNode(webgpu_engine::Context& context, const RouteRenderNodeSettings& settings)
    : Node({
        InputSocket(*this, "route", data_type<const RouteInfo*>()),
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

    const RouteInfo* routeInfo = std::get<data_type<const RouteInfo*>()>(input_socket("route").get_connected_data());

    m_context->track_renderer()->clear_tracks();
    m_context->track_renderer()->add_world_positions_with_colored_segments(routeInfo->path_one, routeInfo->colors_one);
    m_context->track_renderer()->add_world_positions_with_colored_segments(routeInfo->path_two, routeInfo->colors_two);
    m_context->track_renderer()->add_world_positions_with_colored_segments(routeInfo->path_three, routeInfo->colors_three);
    m_context->track_renderer()->add_world_positions_for_control_points(routeInfo->points);
    m_context->request_redraw();

    complete_run();
}

} // namespace webgpu_compute::nodes
