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

void AppAlertModel::clear() {
    beginResetModel();
    rows_.clear();
    endResetModel();
    emit badgeCountChanged();
}

} // namespace micecam::ui
