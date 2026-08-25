#pragma once

#include "nucleus/event_parameter.h"

#include <QObject>

#include <nucleus/camera/AbstractDepthTester.h>
#include <nucleus/camera/Definition.h>

namespace webgpu_app {

class InputMapper;

class MousePicker : public QObject {
    Q_OBJECT

public:
    MousePicker();

    void set_enabled(bool enabled);
    void set_input_mapper(InputMapper* input_mapper);
    void set_camera(const nucleus::camera::Definition* camera);
    void set_depth_tester(nucleus::camera::AbstractDepthTester* depth_tester);
    void connect_mouse_picker();
    void save_gpx();
    void clear_gpx();
    bool enabled() const;

    signals:
        void point_picked(const glm::dvec3& position);

private slots:
    void mouse_press(const nucleus::event_parameter::Mouse& event);

private:
    InputMapper* m_input_mapper = nullptr;
    const nucleus::camera::Definition* m_camera = nullptr;
    nucleus::camera::AbstractDepthTester* m_depth_tester = nullptr;

    bool m_enabled = false;

    std::vector<glm::dvec3> m_picked_points;

    const std::string m_gpx_path = "../../apps/webgpu_app/resources/gpx/picked_track.gpx";;
};

} // namespace webgpu_app