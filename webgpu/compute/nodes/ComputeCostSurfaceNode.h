
#pragma once

#include "Node.h"
#include "types/CostCell.h"

namespace webgpu_compute::nodes {

class ComputeCostSurfaceNode : public Node {
    Q_OBJECT

public:
    NODE_TYPE_NAME(ComputeCostSurfaceNode)

    struct ComputeCostSurfaceNodeSettings {

    };

    ComputeCostSurfaceNode();
    ComputeCostSurfaceNode(const ComputeCostSurfaceNodeSettings& settings);

    void interpolateLine(int x1, int y1, int oldX, int oldY, Way way);
    void calculateSlope(float cellSize);
    void calculateCost();

    void set_settings(const ComputeCostSurfaceNodeSettings& settings);
    const ComputeCostSurfaceNodeSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

    void check_progress_and_emit_signals();

    CostGrid m_cost_grid;
    uint32_t width;
    uint32_t height;

public slots:
    void run_impl() override;

private:
    ComputeCostSurfaceNodeSettings m_settings;

};

} // namespace webgpu_compute::nodes
