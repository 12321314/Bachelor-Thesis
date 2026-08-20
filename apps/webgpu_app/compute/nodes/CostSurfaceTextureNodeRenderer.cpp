#include "CostSurfaceTextureNodeRenderer.h"

#include <imgui.h>

namespace webgpu_app {
namespace nodes = webgpu_compute::nodes;

CostSurfaceTextureNodeRenderer::CostSurfaceTextureNodeRenderer(
    const std::string& name,
    nodes::CostSurfaceTextureNode& node)
    : NodeRenderer(name, node)
    , m_node(&node)
    , m_options({
          { "Cost Surface", { "Cost" } },
          { "Paths", { "Paths" } },
          { "Dangerous Cliffs", { "Cliffs" } },
          { "No Overlay", { "Nothing" } },
      })
{
}

void CostSurfaceTextureNodeRenderer::render_settings_content()
{
    std::string combo_items;
    for (const auto& opt : m_options)
        combo_items += opt.name + '\0';
    combo_items += '\0';

    if (ImGui::Combo("Overlay", &m_selected_index, combo_items.c_str())) {
        m_node->set_settings(m_options[m_selected_index].settings);
        m_node->rerun();
    }
}

} // namespace webgpu_app