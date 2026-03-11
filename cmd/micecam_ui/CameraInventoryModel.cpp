#include "CameraInventoryModel.h"

CameraInventoryModel::CameraInventoryModel(QObject* parent)
    : QAbstractListModel(parent) {}

int CameraInventoryModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_devices.size();
}

QVariant CameraInventoryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_devices.size()) {
        return {};
    }

    const CaptureDeviceDescriptor& device = m_devices.at(index.row());
    switch (role) {
    case DeviceIdRole:
        return device.deviceId;
    case BackendIdRole:
        return device.backendId;
    case NameRole:
    case Qt::DisplayRole:
        return device.displayName;
    case DeviceIndexRole:
        return device.deviceIndex;
    case AvailableRole:
        return device.available;
    case ResolutionsRole:
        return device.supportedResolutions;
    case FpsRole: {
        QVariantList fpsValues;
        for (const int fps : device.supportedFps) {
            fpsValues.push_back(fps);
        }
        return fpsValues;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> CameraInventoryModel::roleNames() const {
    return {
        {DeviceIdRole, "deviceId"},
        {BackendIdRole, "backendId"},
        {NameRole, "name"},
        {DeviceIndexRole, "deviceIndex"},
        {AvailableRole, "available"},
        {ResolutionsRole, "supportedResolutions"},
        {FpsRole, "supportedFps"},
    };
}

void CameraInventoryModel::setDevices(const QList<CaptureDeviceDescriptor>& devices) {
    beginResetModel();
    m_devices = devices;
    endResetModel();
}

QList<CaptureDeviceDescriptor> CameraInventoryModel::devices() const {
    return m_devices;
}

CaptureDeviceDescriptor CameraInventoryModel::deviceAt(int index) const {
    if (index < 0 || index >= m_devices.size()) {
        return {};
    }
    return m_devices.at(index);
}
