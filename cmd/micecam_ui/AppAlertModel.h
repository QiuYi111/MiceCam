#pragma once

#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace micecam::ui {

struct AlertRow {
    int severity = 0;
    QString title;
    QString source;
    QString relativeTime;
};

class AppAlertModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int badgeCount READ badgeCount NOTIFY badgeCountChanged)
public:
    enum AlertRoles {
        SeverityRole = Qt::UserRole + 1,
        TitleRole,
        SourceRole,
        RelativeTimeRole
    };
    Q_ENUM(AlertRoles)

    explicit AppAlertModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int badgeCount() const;
    void replaceRows(std::vector<AlertRow> rows);
    void clear();

signals:
    void badgeCountChanged();

private:
    std::vector<AlertRow> rows_;
};

} // namespace micecam::ui
