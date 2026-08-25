#include "GPXPickedTrackNode.h"

#include "nucleus/track/GPX.h"
#include <QDebug>
#include <QString>

namespace webgpu_compute::nodes {

GPXPickedTrackNode::GPXPickedTrackNode()
    : Node({},
          {
              OutputSocket(*this, "region", data_type<const radix::geometry::Aabb<3, double>*>(), [this]() { return &m_output_region; }),
              OutputSocket(*this, "points", data_type<const nucleus::track::Segment*>(), [this]() { return &m_points; }),
          })
{
}

void GPXPickedTrackNode::run_impl()
{
    if (m_settings.enable_caching && m_has_cached && m_settings.file_path == m_cached_path) {
        complete_run();
        return;
    }

    std::unique_ptr<nucleus::track::Gpx> gpx = nucleus::track::parse(QString::fromStdString(m_settings.file_path));
    if (!gpx) {
        fail_run("could not parse GPX file: " + m_settings.file_path);
        return;
    }

    m_output_region = nucleus::track::compute_world_aabb(*gpx);
    if (gpx->track.empty()) {
        gpx->add_new_segment();
    }
    m_points = gpx->track.back();
    m_cached_path = m_settings.file_path;
    m_has_cached = true;

    double padding = m_settings.region_padding;

    m_output_region.min.x -= padding / 2;
    m_output_region.min.y -= padding / 2;

    m_output_region.max.x += padding / 2;
    m_output_region.max.y += padding / 2;

    qInfo() << Qt::fixed << "gpx region=[(" << m_output_region.min.x << ", " << m_output_region.min.y << "), (" << m_output_region.max.x << ", "
             << m_output_region.max.y << ")]";

    complete_run();
}

void GPXPickedTrackNode::serialize_settings(QJsonObject& out) const
{
    out["file_path"] = QString::fromStdString(m_settings.file_path);
    out["enable_caching"] = m_settings.enable_caching;
}

void GPXPickedTrackNode::deserialize_settings(const QJsonObject& in)
{
    if (in.contains("file_path"))
        m_settings.file_path = in["file_path"].toString().toStdString();
    if (in.contains("enable_caching"))
        m_settings.enable_caching = in["enable_caching"].toBool(m_settings.enable_caching);
}

} // namespace webgpu_compute::nodes
