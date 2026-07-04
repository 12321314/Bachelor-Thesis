
#pragma once

#include "Node.h"
#include "nucleus/tile/TileLoadService.h"

namespace webgpu_compute::nodes {

class CreateHeightGrid : public Node {
    Q_OBJECT

public:
    NODE_TYPE_NAME(CreateHeightGrid)

    struct CreateHeightGridSettings {

    };

    CreateHeightGrid();
    CreateHeightGrid(const CreateHeightGridSettings& settings);

    float decodeHeight(const QColor& c);

    void set_settings(const CreateHeightGridSettings& settings);
    const CreateHeightGridSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

    void check_progress_and_emit_signals();

public slots:
    void run_impl() override;

private:
    CreateHeightGridSettings m_settings;
};

} // namespace webgpu_compute::nodes
