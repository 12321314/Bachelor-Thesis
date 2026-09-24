#pragma once

#include "NodeRenderer.h"
#include <string>
#include <vector>
#include <webgpu/compute/nodes/CalculateRouteNode.h>

namespace webgpu_app {
namespace nodes = webgpu_compute::nodes;

class CalculateRouteNodeRenderer : public NodeRenderer {
public:
    struct TileSourceOption {
        std::string name;
        nodes::CalculateRouteNode::CalculateRouteNodeSettings settings;
    };

    CalculateRouteNodeRenderer(const std::string& name, nodes::CalculateRouteNode& node);
    bool has_settings() const override { return true; }
    void render_settings_content() override;

private:
    nodes::CalculateRouteNode* m_node;
    std::vector<TileSourceOption> m_options;
    int m_selected_index = 0;
};

} // namespace webgpu_app
