
#pragma once

#include "Node.h"
#include "nucleus/track/GPX.h"

namespace webgpu_compute::nodes {

class GPXPickedTrackNode : public Node {
    Q_OBJECT

public:
    NODE_TYPE_NAME(GPXPickedTrackNode)

    struct GPXPickedTrackNodeSettings {
        std::string file_path = "../../apps/webgpu_app/resources/gpx/picked_track.gpx";
        bool enable_caching = true;
        float region_padding = 1000.0f; // meters
    };

    GPXPickedTrackNode();

    void set_settings(const GPXPickedTrackNodeSettings& settings) { m_settings = settings; }
    const GPXPickedTrackNodeSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

public slots:
    void run_impl() override;

private:
    GPXPickedTrackNodeSettings m_settings;
    radix::geometry::Aabb<3, double> m_output_region;

    nucleus::track::Segment m_points;
    std::string m_cached_path;
    bool m_has_cached = false;
};

} // namespace webgpu_compute::nodes
