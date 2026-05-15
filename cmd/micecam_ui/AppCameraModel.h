#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <vector>

namespace micecam::ui {

struct CameraRow {
    QString cameraId;
    QString name;
    QStringList resolutionLabels;
    QStringList framerateLabels;
    QStringList formatLabels;
    QString alertMessage;
    double fps = 0.0;
    int dropCount = 0;
    int status = 0;
    bool recording = false;
};

class AppCameraModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum CameraRoles {
        CameraIdRole = Qt::UserRole + 1,
        NameRole,
        FpsRole,
        DropCountRole,
        IsRecordingRole,
        StatusRole,
        AlertMessageRole,
        ResolutionOptionsRole,
        FramerateOptionsRole,
        FormatOptionsRole
    };
    Q_ENUM(CameraRoles)

    explicit AppCameraModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void replaceRows(std::vector<CameraRow> rows);
    Q_INVOKABLE QVariantMap get(int row) const;

private:
    std::vector<CameraRow> rows_;
};

} // namespace micecam::ui
