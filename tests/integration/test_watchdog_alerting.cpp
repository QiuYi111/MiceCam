#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "domain/AlertRecord.h"
#include "infrastructure/AlertManager.h"
#include "infrastructure/Watchdog.h"

using namespace micecam;

namespace {

class CountingObserver : public api::WatchdogObserver {
public:
    void on_alert(const domain::AlertRecord& alert) override {
        alert_count++;
        last_type = alert.type;
    }
    int alert_count = 0;
    domain::AlertType last_type = domain::AlertType::PIPELINE_STALL;
};

} // namespace

TEST(WatchdogAlerting, WatchdogFiresOnStall) {
    infrastructure::AlertManager mgr;
    CountingObserver obs;
    mgr.register_observer(&obs);

    infrastructure::Watchdog wd(mgr);
    wd.set_timeout_s(1);
    wd.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    wd.stop();
    EXPECT_GT(obs.alert_count, 0);
}

TEST(WatchdogAlerting, AlertManagerDedupWorks) {
    infrastructure::AlertManager mgr;
    mgr.set_dedup_cooldown_ms(10000);
    CountingObserver obs;
    mgr.register_observer(&obs);

    domain::AlertRecord alert;
    alert.type = domain::AlertType::PIPELINE_STALL;
    alert.severity = domain::AlertSeverity::RED;
    alert.stream_id = "test";

    mgr.emit(alert);
    mgr.emit(alert);
    mgr.emit(alert);

    EXPECT_EQ(obs.alert_count, 1);
}

TEST(WatchdogAlerting, AlertManagerMultipleObservers) {
    infrastructure::AlertManager mgr;
    CountingObserver obs1, obs2;
    mgr.register_observer(&obs1);
    mgr.register_observer(&obs2);

    domain::AlertRecord alert;
    alert.type = domain::AlertType::ENCODER_FALLBACK;
    alert.stream_id = "test";

    mgr.emit(alert);

    EXPECT_EQ(obs1.alert_count, 1);
    EXPECT_EQ(obs2.alert_count, 1);
}
