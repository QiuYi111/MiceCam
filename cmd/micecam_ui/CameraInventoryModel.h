#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QObject>

#include "RecordingSetup.h"

class CameraInventoryModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum CameraRoles {
        DeviceIdRole = Qt::UserRole + 1,
        BackendIdRole,
        NameRole,
        DeviceIndexRole,
        AvailableRole,
        ResolutionsRole,
        FpsRole
    };

    explicit CameraInventoryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDevices(const QList<CaptureDeviceDescriptor>& devices);
    QList<CaptureDeviceDescriptor> devices() const;
    CaptureDeviceDescriptor deviceAt(int index) const;

private:
    QList<CaptureDeviceDescriptor> m_devices;
};
