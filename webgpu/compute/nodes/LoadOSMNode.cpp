#include "LoadOSMNode.h"
#include "nucleus/srs.h"
#include "types/Way.h"
#include "util.h"

#include <QColor>
#include <QImage>

#include <QDebug>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <set>

namespace webgpu_compute::nodes {

LoadOSMNode::LoadOSMNode(const LoadOSMNodeSettings& settings)
: Node(
      {
          InputSocket(*this, "region aabb", data_type<const radix::geometry::Aabb<2, double>*>()),
      },
      {
          OutputSocket(*this, "OSM ways", data_type<const std::vector<Way>*>(), [this]() { return &m_ways;}),
      })
{
    set_settings(settings);
}

LoadOSMNode::LoadOSMNode()
    : LoadOSMNode(LoadOSMNodeSettings())
{
}

void LoadOSMNode::run_impl()
{
    const auto region_aabb = std::get<data_type<const radix::geometry::Aabb<2, double>*>()>(input_socket("region aabb").get_connected_data());

    glm::dvec2 lat_lon_min = nucleus::srs::world_to_lat_long(region_aabb->min);
    glm::dvec2 lat_lon_max = nucleus::srs::world_to_lat_long(region_aabb->max);

    QString query = QString(R"(
    [out:json];
    (
      way["highway"](%1,%2,%3,%4);
      way["landuse"="forest"](%1,%2,%3,%4);
      way["natural"="water"](%1,%2,%3,%4);
      way["natural"="wood"](%1,%2,%3,%4);
      way["waterway"](%1,%2,%3,%4);
    );
    out geom;
    )")
    .arg(lat_lon_min.x)
    .arg(lat_lon_min.y)
    .arg(lat_lon_max.x)
    .arg(lat_lon_max.y);

    QNetworkAccessManager* manager = new QNetworkAccessManager(this);

    QNetworkRequest req(
        QUrl("https://overpass-api.de/api/interpreter"));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    manager->post(req, query.toUtf8());

    connect(manager,&QNetworkAccessManager::finished,this,&LoadOSMNode::parse_OSM);

    complete_run();
}

std::vector<Way> LoadOSMNode::parse_OSM(QNetworkReply* reply) {

    QByteArray data = reply->readAll();

    QJsonDocument doc = QJsonDocument::fromJson(data);

    QJsonObject root = doc.object();

    QJsonArray elements = root["elements"].toArray();

    for (const auto& el_value : elements)
    {
        QJsonObject el = el_value.toObject();

        if (el["type"] != "way") continue;
        if (!el.contains("geometry")) continue;

        Way way;

        // tags
        if (el.contains("tags")) {
            QJsonObject tags = el["tags"].toObject();

            if (tags.contains("highway"))
                way.path = true;

            if ((tags.contains("landuse") && tags["landuse"] == "forest") ||
                (tags.contains("natural") && tags["natural"] == "wood"))
                way.forest = true;

            if (tags.contains("waterway") && tags["waterway"] == "stream")
                way.stream = true;

            if (tags.contains("waterway") && tags["waterway"] == "river")
                way.river = true;

            if (tags.contains("natural") && tags["natural"] == "water")
                way.waterbody = true;
        }

        // points
        QJsonArray geometry = el["geometry"].toArray();

        for (const auto& p_value : geometry)
        {
            QJsonObject p = p_value.toObject();

            way.points.emplace_back(p["lat"].toDouble(), p["lon"].toDouble());
        }

        m_ways.push_back(way);
    }

    return m_ways;
}

void LoadOSMNode::set_settings(const LoadOSMNodeSettings& settings)
{
    m_settings = settings;
}

void LoadOSMNode::check_progress_and_emit_signals()
{
    complete_run();
}

void LoadOSMNode::serialize_settings(QJsonObject& out) const
{
}

void LoadOSMNode::deserialize_settings(const QJsonObject& in)
{
}

} // namespace webgpu_compute::nodes


