#include "ComputeCostSurfaceNodeRenderer.h"

#include <imgui.h>

namespace webgpu_app {
namespace nodes = webgpu_compute::nodes;

ComputeCostSurfaceNodeRenderer::ComputeCostSurfaceNodeRenderer(
    const std::string& name,
    nodes::ComputeCostSurfaceNode& node)
    : NodeRenderer(name, node)
    , m_node(&node)
{
}

void ComputeCostSurfaceNodeRenderer::render_settings_content()
{
    nodes::ComputeCostSurfaceNode::ComputeCostSurfaceNodeSettings settings = m_node->get_settings();
    static uint32_t forest_influence = settings.m_forest_influence;
    static uint32_t path_influence = settings.m_path_influence;
    static uint32_t stream_influence = settings.m_stream_influence;
    static uint32_t river_influence = settings.m_river_influence;

    const uint32_t min = 0;
    const uint32_t max = 10;

    ImGui::SliderScalar("Path Influence", ImGuiDataType_U32, &path_influence, &min, &max);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        settings.m_path_influence = path_influence;
        m_node->set_settings(settings);
        m_node->rerun();
    }
    ImGui::SliderScalar("Forest Influence", ImGuiDataType_U32,&forest_influence,&min,&max);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        settings.m_forest_influence = forest_influence;
        m_node->set_settings(settings);
        m_node->rerun();
    }
    ImGui::SliderScalar("Stream Influence", ImGuiDataType_U32,&stream_influence,&min,&max);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        settings.m_stream_influence = stream_influence;
        m_node->set_settings(settings);
        m_node->rerun();
    }
    ImGui::SliderScalar("River Influence", ImGuiDataType_U32,&river_influence,&min,&max);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        settings.m_river_influence = river_influence;
        m_node->set_settings(settings);
        m_node->rerun();
    }
}

} // namespace webgpu_app