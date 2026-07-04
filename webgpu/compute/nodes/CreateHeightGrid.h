
#pragma once

#include "Node.h"
#include "nucleus/tile/TileLoadService.h"

namespace webgpu_compute::nodes {

class CreateHeightGrid : public Node {
    Q_OBJECT

public:
    NODE_TYPE_NAME(CreateHeightGrid)

    struct CreateHeightGridSettings {
        std::string tile_path = "https://alpinemaps.cg.tuwien.ac.at/tiles/at_dtm_alpinemaps/";
        nucleus::tile::TileLoadService::UrlPattern url_pattern = nucleus::tile::TileLoadService::UrlPattern::ZXY;
        std::string file_extension = ".png";
    };

    CreateHeightGrid();
    CreateHeightGrid(const CreateHeightGridSettings& settings);

    void on_single_tile_received(const nucleus::tile::Data& tile);

    void set_settings(const CreateHeightGridSettings& settings);
    const CreateHeightGridSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

    void check_progress_and_emit_signals();

public slots:
    void run_impl() override;

private:
    CreateHeightGridSettings m_settings;
    std::unique_ptr<nucleus::tile::TileLoadService> m_tile_loader;
    size_t m_num_signals_received = 0;
    size_t m_num_tiles_unavailable = 0;
    size_t m_num_tiles_requested = 0;
    std::vector<QByteArray> m_received_tile_textures;
    std::vector<radix::tile::Id> m_requested_tile_ids;
};

} // namespace webgpu_compute::nodes
