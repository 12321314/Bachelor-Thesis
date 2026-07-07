
#pragma once

#include "Node.h"
#include "types/Way.h"

#include <QNetworkReply>

namespace webgpu_compute::nodes {

class LoadOSMNode : public Node {
    Q_OBJECT

public:
    NODE_TYPE_NAME(LoadOSMNode)

    struct LoadOSMNodeSettings {

    };

    LoadOSMNode();
    LoadOSMNode(const LoadOSMNodeSettings& settings);

    std::vector<Way> parse_OSM(QNetworkReply* reply);

    void set_settings(const LoadOSMNodeSettings& settings);
    const LoadOSMNodeSettings& get_settings() const { return m_settings; }
    void serialize_settings(QJsonObject& out) const override;
    void deserialize_settings(const QJsonObject& in) override;

    void check_progress_and_emit_signals();

    std::vector<Way> m_ways;

public slots:
    void run_impl() override;

private:
    LoadOSMNodeSettings m_settings;

};

} // namespace webgpu_compute::nodes
