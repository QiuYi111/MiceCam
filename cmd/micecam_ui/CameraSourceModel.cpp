#include "CameraSourceModel.h"

#include <algorithm>

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
        case PluginVersionRole: return QString::fromStdString(row.source.plugin_version);
        case PluginApiVersionRole: return static_cast<int>(row.source.plugin_api_version);
        case DiagnosticsMessageRole: return QString::fromStdString(row.source.diagnostics_message);
        case RestartRequiredRole: return row.source.restart_required;
        case AvailableDeviceCountRole: return availableDeviceCount(row);
        case IsExpandedRole: return row.is_expanded;
        case DevicesRole: return devicesToList(row);
        case SourceTypeLabelRole: return sourceTypeLabel(row.source.source_type);
        case StatusLabelRole: return diagnosticsLabel(row.source.diagnostics_state);
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
    roles[PluginVersionRole] = "pluginVersion";
    roles[PluginApiVersionRole] = "pluginApiVersion";
    roles[DiagnosticsMessageRole] = "diagnosticsMessage";
    roles[RestartRequiredRole] = "restartRequired";
    roles[AvailableDeviceCountRole] = "availableDeviceCount";
    roles[IsExpandedRole] = "isExpanded";
    roles[DevicesRole] = "devices";
    roles[SourceTypeLabelRole] = "sourceTypeLabel";
    roles[StatusLabelRole] = "statusLabel";
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
    std::stable_sort(rows_.begin(), rows_.end(), [](const SourceRow& a, const SourceRow& b) {
        auto tier = [](const SourceRow& row) {
            if (row.source.enabled && row.source.diagnostics_state == domain::PluginDiagnosticsState::OK &&
                availableDeviceCount(row) > 0) {
                return 0;
            }
            if (row.source.enabled && row.source.diagnostics_state == domain::PluginDiagnosticsState::OK) {
                return 1;
            }
            if (row.source.enabled) {
                return 2;
            }
            return 3;
        };
        const int ta = tier(a);
        const int tb = tier(b);
        if (ta != tb) return ta < tb;
        if (a.source.source_type != b.source.source_type) {
            return a.source.source_type == domain::PluginSourceType::BUNDLED;
        }
        return a.source.source_name < b.source.source_name;
    });
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
    return deviceToMap(row.source, d);
}

QVariantMap CameraSourceModel::getSourceAt(int sourceIndex) const {
    QVariantMap map;
    if (sourceIndex < 0 || sourceIndex >= static_cast<int>(rows_.size()))
        return map;
    const auto& row = rows_[sourceIndex];
    map["sourceId"] = QString::fromStdString(row.source.source_id);
    map["sourceName"] = QString::fromStdString(row.source.source_name);
    map["sourceType"] = static_cast<int>(row.source.source_type);
    map["sourceTypeLabel"] = sourceTypeLabel(row.source.source_type);
    map["pluginVersion"] = QString::fromStdString(row.source.plugin_version);
    map["pluginApiVersion"] = static_cast<int>(row.source.plugin_api_version);
    map["enabled"] = row.source.enabled;
    map["diagnostics"] = static_cast<int>(row.source.diagnostics_state);
    map["diagnosticsMessage"] = QString::fromStdString(row.source.diagnostics_message);
    map["statusLabel"] = diagnosticsLabel(row.source.diagnostics_state);
    map["restartRequired"] = row.source.restart_required;
    map["deviceCount"] = static_cast<int>(row.devices.size());
    map["availableDeviceCount"] = availableDeviceCount(row);
    map["isExpanded"] = row.is_expanded;
    map["devices"] = devicesToList(row);
    return map;
}

int CameraSourceModel::availableDeviceCount(const SourceRow& row) {
    return static_cast<int>(std::count_if(row.devices.begin(), row.devices.end(), [](const auto& device) {
        return device.status.empty() || device.status == "available" || device.status == "connected";
    }));
}

QString CameraSourceModel::diagnosticsLabel(domain::PluginDiagnosticsState state) {
    switch (state) {
        case domain::PluginDiagnosticsState::OK: return QStringLiteral("OK");
        case domain::PluginDiagnosticsState::MISSING: return QStringLiteral("Missing");
        case domain::PluginDiagnosticsState::DISABLED: return QStringLiteral("Disabled");
        case domain::PluginDiagnosticsState::ERROR: return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

QString CameraSourceModel::sourceTypeLabel(domain::PluginSourceType type) {
    switch (type) {
        case domain::PluginSourceType::BUNDLED: return QStringLiteral("Bundled");
        case domain::PluginSourceType::LINKED: return QStringLiteral("Linked");
    }
    return QStringLiteral("Unknown");
}

QVariantMap CameraSourceModel::deviceToMap(const domain::PluginSource& source,
                                           const domain::PluginDeviceInfo& d) {
    QVariantMap map;
    map["deviceId"] = QString::fromStdString(d.device_id);
    map["cameraId"] = QString::fromStdString(d.device_id);
    map["displayName"] = QString::fromStdString(d.display_name);
    map["name"] = QString::fromStdString(d.display_name);
    map["pluginId"] = QString::fromStdString(d.plugin_id);
    map["sourceId"] = QString::fromStdString(source.source_id);
    map["sourceName"] = QString::fromStdString(source.source_name);
    map["status"] = QString::fromStdString(d.status);
    map["statusCode"] = (d.status.empty() || d.status == "available" || d.status == "connected") ? 0 : 2;
    map["supportsRaw"] = d.supports_raw;
    map["supportsMjpeg"] = d.supports_mjpeg;
    map["supportsH264"] = d.supports_h264;
    map["supportsH265"] = d.supports_h265;
    map["maxWidth"] = d.max_width;
    map["maxHeight"] = d.max_height;
    map["maxFramerate"] = d.max_framerate;
    map["fps"] = d.max_framerate;
    map["dropCount"] = 0;
    map["isRecording"] = false;
    map["exclusiveResourceId"] = d.exclusive_resource_id
        ? QString::fromStdString(*d.exclusive_resource_id)
        : QString();
    map["hasDiagnostics"] = d.has_diagnostics;
    map["diagnosticsCode"] = QString::fromStdString(d.diagnostics_code);
    map["diagnosticsMessage"] = QString::fromStdString(d.diagnostics_message);
    return map;
}

QVariantList CameraSourceModel::devicesToList(const SourceRow& row) {
    QVariantList list;
    for (const auto& device : row.devices) {
        list.append(deviceToMap(row.source, device));
    }
    return list;
}

} // namespace micecam::ui
