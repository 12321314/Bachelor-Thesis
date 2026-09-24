#include "CalculateRouteNodeRenderer.h"

#include <imgui.h>

namespace webgpu_app {
namespace nodes = webgpu_compute::nodes;

CalculateRouteNodeRenderer::CalculateRouteNodeRenderer(
    const std::string& name,
    nodes::CalculateRouteNode& node)
    : NodeRenderer(name, node)
    , m_node(&node)
{
}

void CalculateRouteNodeRenderer::render_settings_content()
{
    nodes::CalculateRouteNode::CalculateRouteNodeSettings settings = m_node->get_settings();
    static uint32_t number_of_routes = settings.number_of_routes;

    const uint32_t min_routes = 1;
    const uint32_t max_routes = 3;
    ImGui::SliderScalar("Zoom level", ImGuiDataType_U32, &number_of_routes, &min_routes, &max_routes);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        settings.number_of_routes = number_of_routes;
        m_node->set_settings(settings);
        m_node->rerun();
    }
}

} // namespace webgpu_app