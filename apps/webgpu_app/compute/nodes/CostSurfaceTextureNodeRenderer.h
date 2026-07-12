#pragma once

#include "NodeRenderer.h"
#include <string>
#include <vector>
#include <webgpu/compute/nodes/CostSurfaceTextureNode.h>

namespace webgpu_app {
namespace nodes = webgpu_compute::nodes;

class CostSurfaceTextureNodeRenderer : public NodeRenderer {
public:
    struct TileSourceOption {
        std::string name;
        nodes::CostSurfaceTextureNode::CostSurfaceTextureNodeSettings settings;
    };

    CostSurfaceTextureNodeRenderer(const std::string& name, nodes::CostSurfaceTextureNode& node);
    bool has_settings() const override { return true; }
    void render_settings_content() override;

private:
    nodes::CostSurfaceTextureNode* m_node;
    std::vector<TileSourceOption> m_options;
    int m_selected_index = 0;
};

} // namespace webgpu_app
