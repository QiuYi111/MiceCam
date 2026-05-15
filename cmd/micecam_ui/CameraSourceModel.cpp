#include "CameraSourceModel.h"

namespace micecam::ui {

CameraSourceModel::CameraSourceModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CameraSourceModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(rows_.size());
}

QVariant CameraSourceModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(rows_.size()))
        return QVariant();

    const auto& row = rows_[index.row()];
    switch (role) {
        case SourceIdRole: return QString::fromStdString(row.source.source_id);
        case SourceNameRole: return QString::fromStdString(row.source.source_name);
        case SourceTypeRole: return static_cast<int>(row.source.source_type);
        case DeviceCountRole: return static_cast<int>(row.devices.size());
        case EnabledRole: return row.source.enabled;
        case DiagnosticsRole: return static_cast<int>(row.source.diagnostics_state);
    }
    return QVariant();
}

QHash<int, QByteArray> CameraSourceModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[SourceIdRole] = "sourceId";
    roles[SourceNameRole] = "sourceName";
    roles[SourceTypeRole] = "sourceType";
    roles[DeviceCountRole] = "deviceCount";
    roles[EnabledRole] = "enabled";
    roles[DiagnosticsRole] = "diagnostics";
    return roles;
}

void CameraSourceModel::populateFromSources(
    const std::vector<domain::PluginSource>& sources,
    const std::vector<domain::PluginDeviceInfo>& devices) {

    beginResetModel();
    rows_.clear();
    for (const auto& src : sources) {
        SourceRow row;
        row.source = src;
        for (const auto& d : devices) {
            if (d.plugin_id == src.source_id) {
                row.devices.push_back(d);
            }
        }
        rows_.push_back(std::move(row));
    }
    endResetModel();
}

QVariantMap CameraSourceModel::getDeviceAt(int sourceIndex, int deviceIndex) const {
    QVariantMap map;
    if (sourceIndex < 0 || sourceIndex >= static_cast<int>(rows_.size()))
        return map;
    const auto& row = rows_[sourceIndex];
    if (deviceIndex < 0 || deviceIndex >= static_cast<int>(row.devices.size()))
        return map;
    const auto& d = row.devices[deviceIndex];
    map["deviceId"] = QString::fromStdString(d.device_id);
    map["displayName"] = QString::fromStdString(d.display_name);
    map["pluginId"] = QString::fromStdString(d.plugin_id);
    map["status"] = QString::fromStdString(d.status);
    map["supportsRaw"] = d.supports_raw;
    map["supportsMjpeg"] = d.supports_mjpeg;
    map["supportsH264"] = d.supports_h264;
    map["supportsH265"] = d.supports_h265;
    map["maxWidth"] = d.max_width;
    map["maxHeight"] = d.max_height;
    map["maxFramerate"] = d.max_framerate;
    map["hasDiagnostics"] = d.has_diagnostics;
    map["diagnosticsCode"] = QString::fromStdString(d.diagnostics_code);
    map["diagnosticsMessage"] = QString::fromStdString(d.diagnostics_message);
    return map;
}

} // namespace micecam::ui
