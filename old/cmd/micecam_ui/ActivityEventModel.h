#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QVector>

namespace micecam_ui {

struct ActivityEvent {
    QString severity;
    QString category;
    QString message;
    QString relatedPath;
    QDateTime timestamp = QDateTime::currentDateTime();
};

class ActivityEventModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        SeverityRole = Qt::UserRole + 1,
        CategoryRole,
        MessageRole,
        RelatedPathRole,
        TimestampRole,
    };

    explicit ActivityEventModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void appendEvent(const ActivityEvent& event);
    void clear();

private:
    QVector<ActivityEvent> m_events;
};

}  // namespace micecam_ui
