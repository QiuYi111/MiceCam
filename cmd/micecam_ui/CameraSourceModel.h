#pragma once

#include <QAbstractListModel>
#include <QHash>
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
        DiagnosticsRole
    };
    Q_ENUM(SourceRoles)

    explicit CameraSourceModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void populateFromSources(const std::vector<domain::PluginSource>& sources,
                             const std::vector<domain::PluginDeviceInfo>& devices);
    Q_INVOKABLE QVariantMap getDeviceAt(int sourceIndex, int deviceIndex) const;

private:
    struct SourceRow {
        domain::PluginSource source;
        std::vector<domain::PluginDeviceInfo> devices;
    };
    std::vector<SourceRow> rows_;
};

} // namespace micecam::ui
