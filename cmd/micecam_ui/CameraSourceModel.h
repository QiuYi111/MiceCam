#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <vector>

#include "domain/PluginDeviceInfo.h"
#include "domain/PluginSource.h"

namespace micecam::ui {

class CameraSourceModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum SourceRoles {
        SourceIdRole = Qt::UserRole + 1,
        SourceNameRole,
        SourceTypeRole,
        DeviceCountRole,
        EnabledRole,
        DiagnosticsRole,
        PluginVersionRole,
        PluginApiVersionRole,
        DiagnosticsMessageRole,
        RestartRequiredRole,
        AvailableDeviceCountRole,
        IsExpandedRole,
        DevicesRole,
        SourceTypeLabelRole,
        StatusLabelRole
    };
    Q_ENUM(SourceRoles)

    explicit CameraSourceModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void populateFromSources(const std::vector<domain::PluginSource>& sources,
                             const std::vector<domain::PluginDeviceInfo>& devices);
    Q_INVOKABLE QVariantMap getDeviceAt(int sourceIndex, int deviceIndex) const;
    Q_INVOKABLE QVariantMap getSourceAt(int sourceIndex) const;

private:
    struct SourceRow {
        domain::PluginSource source;
        std::vector<domain::PluginDeviceInfo> devices;
        bool is_expanded = true;
    };
    std::vector<SourceRow> rows_;

    static int availableDeviceCount(const SourceRow& row);
    static QString diagnosticsLabel(domain::PluginDiagnosticsState state);
    static QString sourceTypeLabel(domain::PluginSourceType type);
    static QVariantMap deviceToMap(const domain::PluginSource& source,
                                   const domain::PluginDeviceInfo& device);
    static QVariantList devicesToList(const SourceRow& row);
};

} // namespace micecam::ui
