#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "api/micecam/WatchdogObserver.h"
#include "domain/AlertRecord.h"

namespace micecam::infrastructure {

class AlertManager {
public:
    void register_observer(api::WatchdogObserver* observer);
    void unregister_observer(api::WatchdogObserver* observer);
    void emit(const domain::AlertRecord& alert);
    void set_dedup_cooldown_ms(int ms);
    std::vector<domain::AlertRecord> history() const;
    void clear_history();

private:
    bool is_duplicate(const domain::AlertRecord& alert);

    mutable std::mutex mutex_;
    std::vector<domain::AlertRecord> history_;
    std::vector<api::WatchdogObserver*> observers_;
    int dedup_cooldown_ms_ = 5000;

    struct DedupKey {
        domain::AlertType type;
        std::string stream_id;
        bool operator<(const DedupKey& o) const {
            if (type != o.type) return type < o.type;
            return stream_id < o.stream_id;
        }
    };
    std::map<DedupKey, std::chrono::steady_clock::time_point> last_emit_;
};

} // namespace micecam::infrastructure
