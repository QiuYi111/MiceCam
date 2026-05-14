#include "ActivityEventModel.h"

namespace micecam_ui {

ActivityEventModel::ActivityEventModel(QObject* parent) : QAbstractListModel(parent) {}

int ActivityEventModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_events.size());
}

QVariant ActivityEventModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_events.size()) {
        return {};
    }

    const ActivityEvent& event = m_events.at(index.row());
    switch (role) {
    case SeverityRole:
        return event.severity;
    case CategoryRole:
        return event.category;
    case MessageRole:
        return event.message;
    case RelatedPathRole:
        return event.relatedPath;
    case TimestampRole:
        return event.timestamp;
    default:
        return {};
    }
}

QHash<int, QByteArray> ActivityEventModel::roleNames() const {
    return {
        {SeverityRole, "severity"},
        {CategoryRole, "category"},
        {MessageRole, "message"},
        {RelatedPathRole, "relatedPath"},
        {TimestampRole, "timestamp"},
    };
}

void ActivityEventModel::appendEvent(const ActivityEvent& event) {
    constexpr int kMaxEvents = 200;
    if (m_events.size() == kMaxEvents) {
        beginRemoveRows({}, 0, 0);
        m_events.removeFirst();
        endRemoveRows();
    }

    const int row = static_cast<int>(m_events.size());
    beginInsertRows({}, row, row);
    m_events.push_back(event);
    endInsertRows();
}

void ActivityEventModel::clear() {
    if (m_events.isEmpty()) {
        return;
    }
    beginResetModel();
    m_events.clear();
    endResetModel();
}

}  // namespace micecam_ui
