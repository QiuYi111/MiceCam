#pragma once

#include <QAbstractListModel>
#include <QString>
#include <vector>

struct MockCamera {
    QString id;
    QString name;
    double fps;
    int dropCount;
    bool isRecording;
    int status; // 0 = Connected, 1 = Warning, 2 = Disconnected
    QString alertMessage;
};

class MockCameraModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum CameraRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        FpsRole,
        DropCountRole,
        IsRecordingRole,
        StatusRole,
        AlertMessageRole
    };

    explicit MockCameraModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void simulateRecordingStart();
    Q_INVOKABLE void simulateRecordingStop();
    Q_INVOKABLE void simulateAlert(int index, const QString& message, int newStatus);

private:
    std::vector<MockCamera> m_cameras;
};
