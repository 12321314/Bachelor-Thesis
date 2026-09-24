#pragma once

#include "NodeRenderer.h"
#include <string>
#include <vector>
#include <webgpu/compute/nodes/ComputeCostSurfaceNode.h>

namespace webgpu_app {
namespace nodes = webgpu_compute::nodes;

class ComputeCostSurfaceNodeRenderer : public NodeRenderer {
public:
    struct TileSourceOption {
        std::string name;
        nodes::ComputeCostSurfaceNode::ComputeCostSurfaceNodeSettings settings;
    };

    ComputeCostSurfaceNodeRenderer(const std::string& name, nodes::ComputeCostSurfaceNode& node);
    bool has_settings() const override { return true; }
    void render_settings_content() override;

private:
    nodes::ComputeCostSurfaceNode* m_node;
    std::vector<TileSourceOption> m_options;
    int m_selected_index = 0;
};

} // namespace webgpu_app
