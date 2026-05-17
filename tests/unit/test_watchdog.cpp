#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "domain/AlertRecord.h"
#include "infrastructure/AlertManager.h"
#include "infrastructure/Watchdog.h"

using namespace micecam;

namespace {

class AlertCapture : public api::WatchdogObserver {
public:
    void on_alert(const domain::AlertRecord& alert) override {
        last_alert = alert;
        alert_count++;
    }
    domain::AlertRecord last_alert;
    int alert_count = 0;
};

} // namespace

TEST(Watchdog, FeedPreventsAlert) {
    infrastructure::AlertManager mgr;
    AlertCapture capture;
    mgr.register_observer(&capture);

    infrastructure::Watchdog wd(mgr);
    wd.set_timeout_s(1);
    wd.start();

    for (int i = 0; i < 5; i++) {
        wd.feed();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    wd.stop();
    EXPECT_EQ(capture.alert_count, 0);
}

TEST(Watchdog, TimeoutTriggersAlert) {
    infrastructure::AlertManager mgr;
    AlertCapture capture;
    mgr.register_observer(&capture);

    infrastructure::Watchdog wd(mgr);
    wd.set_timeout_s(1);
    wd.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    wd.stop();
    EXPECT_GT(capture.alert_count, 0);
}

TEST(Watchdog, PostAlertContinuesMonitoring) {
    infrastructure::AlertManager mgr;
    AlertCapture capture;
    mgr.register_observer(&capture);

    infrastructure::Watchdog wd(mgr);
    wd.set_timeout_s(1);
    wd.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    wd.feed();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    wd.feed();

    wd.stop();
    EXPECT_GT(capture.alert_count, 0);
}

TEST(Watchdog, StopCleanlyExits) {
    infrastructure::AlertManager mgr;
    infrastructure::Watchdog wd(mgr);
    wd.set_timeout_s(10);
    wd.start();
    wd.stop();
}
