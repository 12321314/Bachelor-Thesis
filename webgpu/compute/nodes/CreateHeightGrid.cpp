#include "CreateHeightGrid.h"
#include "nucleus/srs.h"
#include "util.h"

#include <QColor>
#include <QImage>

#include <QDebug>
#include <QFileInfo>
#include <set>

namespace webgpu_compute::nodes {

CreateHeightGrid::CreateHeightGrid(const CreateHeightGridSettings& settings)
: Node(
      {
          InputSocket(*this, "QImage", data_type<const QImage*>()),
          InputSocket(*this, "tile dimension", data_type<const std::vector<float>*>())
      },
      {
          OutputSocket(*this, "height grid", data_type<const HeightGrid*>(), [this]() { return &m_height_grid;}),
      })
{
    set_settings(settings);
}

CreateHeightGrid::CreateHeightGrid()
    : CreateHeightGrid(CreateHeightGridSettings())
{
}
//TODO rename to CreateHeightGridNode
void CreateHeightGrid::run_impl()
{
    const auto& image = *std::get<data_type<const QImage*>()>(input_socket("QImage").get_connected_data());
    const auto& dimensions = *std::get<data_type<const std::vector<float>*>()>(input_socket("tile dimension").get_connected_data());

    if (image.isNull()) {
        fail_run("invalid image");
        return;
    }

    const int width = image.width();
    const int height = image.height();

    qInfo() << "Creating height grid:" << width << "x" << height;

    m_height_grid.width = width;
    m_height_grid.height = height;

    m_height_grid.cell_size_meters = dimensions[0] / dimensions[1];

    qInfo() << "image width:" << dimensions[0];
    qInfo() << "image dimension:" << dimensions[1];

    m_height_grid.heights.resize(width * height);

    for (int y = 0; y < height; ++y) {

        for (int x = 0; x < width; ++x) {

            QColor c = image.pixelColor(x, y);

            float h = decode_height(c);

            m_height_grid.heights[y * width + x] = h;
        }
    }

    qInfo() << "Height grid finished";

    complete_run();
}

float CreateHeightGrid::decode_height(const QColor& c)
{
    // from height_decode_compute.wgsl

    uint16_t raw = (uint16_t(c.red()) << 8) | uint16_t(c.green());

    constexpr float scaling_factor = 8191.875f / 65535.0f;

    return float(raw) * scaling_factor;
}

void CreateHeightGrid::set_settings(const CreateHeightGridSettings& settings)
{
    m_settings = settings;
}

void CreateHeightGrid::check_progress_and_emit_signals()
{
    complete_run();
}

void CreateHeightGrid::serialize_settings(QJsonObject& out) const
{
}

void CreateHeightGrid::deserialize_settings(const QJsonObject& in)
{
}

} // namespace webgpu_compute::nodes


