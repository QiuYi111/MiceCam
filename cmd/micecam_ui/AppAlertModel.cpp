#include "AppAlertModel.h"

namespace micecam::ui {

AppAlertModel::AppAlertModel(QObject* parent)
    : QAbstractListModel(parent) {}

int AppAlertModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return static_cast<int>(rows_.size());
}

QVariant AppAlertModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(rows_.size()))
        return QVariant();

    const auto& row = rows_[index.row()];
    switch (role) {
        case SeverityRole: return row.severity;
        case TitleRole: return row.title;
        case SourceRole: return row.source;
        case RelativeTimeRole: return row.relativeTime;
    }
    return QVariant();
}

QHash<int, QByteArray> AppAlertModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[SeverityRole] = "severity";
    roles[TitleRole] = "title";
    roles[SourceRole] = "source";
    roles[RelativeTimeRole] = "relativeTime";
    return roles;
}

int AppAlertModel::badgeCount() const {
    return static_cast<int>(rows_.size());
}

void AppAlertModel::replaceRows(std::vector<AlertRow> rows) {
    beginResetModel();
    rows_ = std::move(rows);
    endResetModel();
    emit badgeCountChanged();
}

void AppAlertModel::pushAlert(const QString& title, const QString& source,
                               int severity, const QString& alertId,
                               bool autoDismiss) {
    beginInsertRows(QModelIndex(), static_cast<int>(rows_.size()),
                    static_cast<int>(rows_.size()));
    AlertRow row;
    row.alertId = alertId.isEmpty() ? source : alertId;
    row.severity = severity;
    row.title = title;
    row.source = source;
    row.relativeTime = QStringLiteral("now");
    row.autoDismiss = autoDismiss;
    rows_.push_back(std::move(row));
    endInsertRows();
    emit badgeCountChanged();
}

void AppAlertModel::dismissAlert(const QString& alertId) {
    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        if (rows_[i].alertId == alertId) {
            beginRemoveRows(QModelIndex(), i, i);
            rows_.erase(rows_.begin() + i);
            endRemoveRows();
            emit badgeCountChanged();
            return;
        }
    }
}

void AppAlertModel::dismissBySource(const QString& source) {
    std::vector<QString> ids;
    for (const auto& row : rows_) {
        if (row.source == source) {
            ids.push_back(row.alertId);
        }
    }
    for (const auto& id : ids) {
        dismissAlert(id);
    }
}

void AppAlertModel::clear() {
    beginResetModel();
    rows_.clear();
    endResetModel();
    emit badgeCountChanged();
}

} // namespace micecam::ui
