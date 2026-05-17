#include "AppCameraModel.h"

namespace micecam::ui {

AppCameraModel::AppCameraModel(QObject* parent)
    : QAbstractListModel(parent) {}

int AppCameraModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(rows_.size());
}

QVariant AppCameraModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(rows_.size()))
        return QVariant();

    const auto& row = rows_[index.row()];
    switch (role) {
        case CameraIdRole: return row.cameraId;
        case NameRole: return row.name;
        case FpsRole: return row.fps;
        case DropCountRole: return row.dropCount;
        case IsRecordingRole: return row.recording;
        case StatusRole: return row.status;
        case AlertMessageRole: return row.alertMessage;
        case ResolutionOptionsRole: return row.resolutionLabels;
        case FramerateOptionsRole: return row.framerateLabels;
        case FormatOptionsRole: return row.formatLabels;
        case SourceIdRole: return row.sourceId;
        case SourceGroupRole: return row.sourceGroup;
    }
    return QVariant();
}

QHash<int, QByteArray> AppCameraModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[CameraIdRole] = "cameraId";
    roles[NameRole] = "name";
    roles[FpsRole] = "fps";
    roles[DropCountRole] = "dropCount";
    roles[IsRecordingRole] = "isRecording";
    roles[StatusRole] = "status";
    roles[AlertMessageRole] = "alertMessage";
    roles[ResolutionOptionsRole] = "resolutionOptions";
    roles[FramerateOptionsRole] = "framerateOptions";
    roles[FormatOptionsRole] = "formatOptions";
    roles[SourceIdRole] = "sourceId";
    roles[SourceGroupRole] = "sourceGroup";
    return roles;
}

void AppCameraModel::replaceRows(std::vector<CameraRow> rows) {
    beginResetModel();
    rows_ = std::move(rows);
    endResetModel();
}

QVariantMap AppCameraModel::get(int row) const {
    QVariantMap map;
    if (row < 0 || row >= static_cast<int>(rows_.size()))
        return map;
    const auto& r = rows_[row];
    map["cameraId"] = r.cameraId;
    map["name"] = r.name;
    map["fps"] = r.fps;
    map["dropCount"] = r.dropCount;
    map["isRecording"] = r.recording;
    map["status"] = r.status;
    map["alertMessage"] = r.alertMessage;
    map["resolutionOptions"] = r.resolutionLabels;
    map["framerateOptions"] = r.framerateLabels;
    map["formatOptions"] = r.formatLabels;
    map["sourceId"] = r.sourceId;
    map["sourceGroup"] = r.sourceGroup;
    return map;
}

} // namespace micecam::ui
