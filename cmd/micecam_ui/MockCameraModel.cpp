#include "MockCameraModel.h"

MockCameraModel::MockCameraModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_cameras = {
        {"cam_a", "CAM_A", 29.97, 0, false, 0, ""},
        {"cam_b", "CAM_B", 29.97, 0, false, 0, ""},
        {"cam_c", "CAM_C", 29.97, 0, false, 0, ""},
        {"cam_d", "CAM_D", 18.45, 152, false, 1, "High drop rate detected on CAM_D"},
        {"usb_1", "USB-1", 29.97, 0, false, 0, ""}
    };
}

int MockCameraModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_cameras.size());
}

QVariant MockCameraModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_cameras.size())
        return QVariant();

    const auto &cam = m_cameras[index.row()];
    switch (role) {
        case IdRole: return cam.id;
        case NameRole: return cam.name;
        case FpsRole: return cam.fps;
        case DropCountRole: return cam.dropCount;
        case IsRecordingRole: return cam.isRecording;
        case StatusRole: return cam.status;
        case AlertMessageRole: return cam.alertMessage;
    }
    return QVariant();
}

QHash<int, QByteArray> MockCameraModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "cameraId";
    roles[NameRole] = "name";
    roles[FpsRole] = "fps";
    roles[DropCountRole] = "dropCount";
    roles[IsRecordingRole] = "isRecording";
    roles[StatusRole] = "status";
    roles[AlertMessageRole] = "alertMessage";
    return roles;
}

void MockCameraModel::simulateRecordingStart() {
    beginResetModel();
    for (auto &cam : m_cameras) {
        cam.isRecording = true;
    }
    endResetModel();
}

void MockCameraModel::simulateRecordingStop() {
    beginResetModel();
    for (auto &cam : m_cameras) {
        cam.isRecording = false;
    }
    endResetModel();
}

void MockCameraModel::simulateAlert(int index, const QString& message, int newStatus) {
    if (index >= 0 && index < m_cameras.size()) {
        m_cameras[index].alertMessage = message;
        m_cameras[index].status = newStatus;
        emit dataChanged(createIndex(index, 0), createIndex(index, 0));
    }
}
