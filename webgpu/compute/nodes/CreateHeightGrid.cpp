#include "CreateHeightGrid.h"
#include "util.h"

#include <QDebug>
#include <set>

namespace webgpu_compute::nodes {

CreateHeightGrid::CreateHeightGrid(const CreateHeightGridSettings& settings)
    : Node(
          {
              InputSocket(*this, "tile ids", data_type<const std::vector<radix::tile::Id>*>()),
          },
          {
              OutputSocket(*this, "tile data", data_type<const std::vector<QByteArray>*>(),
                           [this]() { return &m_received_tile_textures; }),
          })
{
    set_settings(settings);
}

CreateHeightGrid::CreateHeightGrid()
    : CreateHeightGrid(CreateHeightGridSettings())
{
}

void CreateHeightGrid::run_impl()
{
    // get tile ids to request
    // TODO maybe make get_input_data a template (so usage would become get_input_data<type>(socket_index))
    const auto& tile_ids = *std::get<data_type<const std::vector<radix::tile::Id>*>()>(input_socket("tile ids").get_connected_data());

    // if input tile ids didnt change from last run
    std::set<radix::tile::Id> new_tile_ids(tile_ids.begin(), tile_ids.end());
    std::set<radix::tile::Id> old_tile_ids(m_requested_tile_ids.begin(), m_requested_tile_ids.end());
    if (new_tile_ids == old_tile_ids) {
        qDebug() << "tiles already requested, use cache";
        check_progress_and_emit_signals();
        return;
    }

    // send request for each tile
    m_received_tile_textures.resize(tile_ids.size());
    m_requested_tile_ids = tile_ids;
    m_num_tiles_requested = m_received_tile_textures.size();
    m_num_tiles_unavailable = 0;
    m_num_signals_received = 0;
    qDebug() << "requesting " << m_num_tiles_requested << " tiles ...";
    for (const auto& tile_id : tile_ids) {
        m_tile_loader->load(tile_id);
    }
}

void CreateHeightGrid::on_single_tile_received(const nucleus::tile::Data& tile)
{
    auto found_it = std::find(m_requested_tile_ids.begin(), m_requested_tile_ids.end(), tile.id);
    if (found_it == m_requested_tile_ids.end()) {
        // received tile id that was not requested
        // this means, we send requested a new set of tiles before the responses for the old ones arrived
        // ignore those, we are only interested in responses to the last set of requested tiles
        return;
    }

    m_num_signals_received++;
    if (tile.network_info.status != nucleus::tile::NetworkInfo::Status::Good) {
        m_num_tiles_unavailable++;
        qWarning() << "failed to load tile id x=" << tile.id.coords.x << ", y=" << tile.id.coords.y << ", zoomlevel=" << tile.id.zoom_level << ": "
                   << (tile.network_info.status == nucleus::tile::NetworkInfo::Status::NotFound ? "Not found" : "Network error");
    } else {
        size_t found_index = found_it - m_requested_tile_ids.begin();
        m_received_tile_textures[found_index] = *tile.data;
    }

    check_progress_and_emit_signals();

    qInfo() << "size:"
             << tile.data->size();

    qInfo() << "first bytes:"
             << tile.data->left(16).toHex();
}

void CreateHeightGrid::set_settings(
    const CreateHeightGridSettings& settings)
{
    m_settings = settings;

    m_requested_tile_ids.clear();

    m_tile_loader =
        std::make_unique<nucleus::tile::TileLoadService>(
            QString::fromStdString(settings.tile_path),
            settings.url_pattern,
            QString::fromStdString(settings.file_extension));

    connect(
        m_tile_loader.get(),
        &nucleus::tile::TileLoadService::load_finished,
        this,
        &CreateHeightGrid::on_single_tile_received);
}

void CreateHeightGrid::check_progress_and_emit_signals()
{
    // when all requests are finished
    if (m_num_signals_received ==
        m_num_tiles_requested) {

        if (m_num_tiles_unavailable > 0) {

            fail_run(
                "failed to load " +
                std::to_string(m_num_tiles_unavailable) +
                " tiles from " +
                m_settings.tile_path);

        } else {

            complete_run();
        }
    }
}

void CreateHeightGrid::serialize_settings(
    QJsonObject& out) const
{
    out["tile_path"] =
        QString::fromStdString(m_settings.tile_path);

    out["url_pattern"] =
        url_pattern_to_string(m_settings.url_pattern);

    out["file_extension"] =
        QString::fromStdString(m_settings.file_extension);
}

void CreateHeightGrid::deserialize_settings(
    const QJsonObject& in)
{
    auto s = m_settings;

    if (in.contains("tile_path"))
        s.tile_path =
            in["tile_path"].toString().toStdString();

    if (in.contains("url_pattern"))
        s.url_pattern =
            url_pattern_from_string(
                in["url_pattern"].toString(),
                s.url_pattern);

    if (in.contains("file_extension"))
        s.file_extension =
            in["file_extension"].toString().toStdString();

    set_settings(s);
}

} // namespace webgpu_compute::nodes


