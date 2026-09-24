#include "MousePicker.h"

#include "InputMapper.h"
#include "nucleus/srs.h"

#include <QDebug>
#include <QFile>
#include <qdir.h>

namespace webgpu_app {

MousePicker::MousePicker()

{

}

void MousePicker::set_enabled(bool enabled)
{
    m_enabled = enabled;
}

void MousePicker::set_input_mapper(InputMapper* input_mapper)
{
    m_input_mapper = input_mapper;
}

void MousePicker::set_camera(const nucleus::camera::Definition* camera)
{
    m_camera = camera;
}

void MousePicker::set_depth_tester(nucleus::camera::AbstractDepthTester* depth_tester)
{
    m_depth_tester = depth_tester;
}

bool MousePicker::enabled() const
{
    return m_enabled;
}

void MousePicker::connect_mouse_picker()
{
    Q_ASSERT(m_input_mapper != nullptr);
    Q_ASSERT(m_camera != nullptr);
    Q_ASSERT(m_depth_tester != nullptr);

    connect(
        m_input_mapper,
        &InputMapper::mouse_pressed,
        this,
        &MousePicker::mouse_press);
}

void MousePicker::mouse_press(const nucleus::event_parameter::Mouse& event)
{
    qInfo() << m_enabled;
    if (!m_enabled)
        return;

    // only react to left mouse button.
    if (!(event.buttons & Qt::LeftButton))
        return;

    const glm::vec2 mouse_position = event.point.position;

    // convert screen coordinates to normalized device coordinates.
    const glm::dvec2 ndc = m_camera->to_ndc(mouse_position);

    // Read the world position from the position buffer.
    const glm::dvec3 world_position = m_depth_tester->position(ndc);

    emit point_picked(world_position);

    m_picked_points.push_back(glm::dvec4{world_position.x, world_position.y, world_position.z, 1.0});

    qInfo() << "picked position: " << world_position.x << ", " << world_position.y << ", " << world_position.z;

    save_gpx();
}

void MousePicker::save_gpx()
{
    QFile file(QString::fromStdString(m_gpx_path));

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open GPX file:"
                   << file.errorString();
        return;
    }

    QTextStream out(&file);

    out << "<?xml version='1.0' encoding='UTF-8'?>\n";
    out << "<gpx version=\"1.1\" creator=\"weBIGeo MousePicker\" "
           "xmlns=\"http://www.topografix.com/GPX/1/1\" "
           "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
           "http://www.topografix.com/GPX/1/1/gpx.xsd\">\n";

    out << "  <metadata>\n";
    out << "    <name>weBIGeo Picked Track</name>\n";
    out << "  </metadata>\n";

    out << "  <trk>\n";
    out << "    <name>weBIGeo Picked Track</name>\n";
    out << "    <trkseg>\n";

    for (const auto& point : m_picked_points) {

        const glm::dvec2 lat_lon = nucleus::srs::world_to_lat_long(glm::dvec2(point.x, point.y));

        const double lat = lat_lon.x;
        const double lon = lat_lon.y;
        const double elevation = point.z;

        out << "      <trkpt lat=\""
            << QString::number(lat, 'f', 6)
            << "\" lon=\""
            << QString::number(lon, 'f', 6)
            << "\">\n";

        out << "        <ele>"
            << QString::number(elevation, 'f', 6)
            << "</ele>\n";

        out << "      </trkpt>\n";
    }

    out << "    </trkseg>\n";
    out << "  </trk>\n";
    out << "</gpx>\n";

    file.close();
}

std::vector<glm::vec4> MousePicker::getPoints()
{
    return m_picked_points;
}

void MousePicker::clear_gpx()
{
    m_picked_points.clear();
    save_gpx();
}

} // namespace webgpu_app