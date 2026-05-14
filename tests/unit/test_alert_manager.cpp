#include <gtest/gtest.h>

#include <atomic>
#include <vector>

#include "api/micecam/WatchdogObserver.h"
#include "domain/AlertRecord.h"
#include "infrastructure/AlertManager.h"

using namespace micecam;

namespace {

class TestObserver : public api::WatchdogObserver {
public:
    void on_alert(const domain::AlertRecord& alert) override {
        alerts_.push_back(alert);
    }
    const std::vector<domain::AlertRecord>& alerts() const { return alerts_; }
    std::atomic<int> call_count{0};
private:
    std::vector<domain::AlertRecord> alerts_;
};

class CountingObserver : public api::WatchdogObserver {
public:
    void on_alert(const domain::AlertRecord&) override {
        count_++;
    }
    int count() const { return count_.load(); }
private:
    std::atomic<int> count_{0};
};

} // namespace

TEST(AlertManager, EmitNotifiesRegisteredObserver) {
    infrastructure::AlertManager mgr;
    auto obs = std::make_shared<TestObserver>();
    mgr.register_observer(obs.get());

    domain::AlertRecord alert;
    alert.type = domain::AlertType::CAMERA_DISCONNECT;
    alert.severity = domain::AlertSeverity::RED;
    alert.stream_id = "cam_a";
    alert.message = "disconnected";

    mgr.emit(alert);
    ASSERT_EQ(obs->alerts().size(), 1u);
    EXPECT_EQ(obs->alerts()[0].type, domain::AlertType::CAMERA_DISCONNECT);
    EXPECT_EQ(obs->alerts()[0].stream_id, "cam_a");
}

TEST(AlertManager, UnregisteredObserverNotNotified) {
    infrastructure::AlertManager mgr;
    auto obs = std::make_shared<TestObserver>();
    mgr.register_observer(obs.get());
    mgr.unregister_observer(obs.get());

    domain::AlertRecord alert;
    alert.type = domain::AlertType::DISK_FULL;
    mgr.emit(alert);

    EXPECT_TRUE(obs->alerts().empty());
}

TEST(AlertManager, MultipleObserversAllNotified) {
    infrastructure::AlertManager mgr;
    TestObserver obs1, obs2;
    mgr.register_observer(&obs1);
    mgr.register_observer(&obs2);

    domain::AlertRecord alert;
    alert.type = domain::AlertType::ENCODER_FALLBACK;
    mgr.emit(alert);

    EXPECT_EQ(obs1.alerts().size(), 1u);
    EXPECT_EQ(obs2.alerts().size(), 1u);
}

TEST(AlertManager, DedupSuppressesRepeatAlerts) {
    infrastructure::AlertManager mgr;
    mgr.set_dedup_cooldown_ms(10000);
    TestObserver obs;
    mgr.register_observer(&obs);

    domain::AlertRecord alert;
    alert.type = domain::AlertType::HIGH_DROP_RATE;
    alert.stream_id = "cam_a";

    mgr.emit(alert);
    mgr.emit(alert);
    mgr.emit(alert);

    EXPECT_EQ(obs.alerts().size(), 1u);
}

TEST(AlertManager, NoDedupForDifferentTypes) {
    infrastructure::AlertManager mgr;
    mgr.set_dedup_cooldown_ms(10000);
    TestObserver obs;
    mgr.register_observer(&obs);

    domain::AlertRecord a1;
    a1.type = domain::AlertType::HIGH_DROP_RATE;
    a1.stream_id = "cam_a";
    mgr.emit(a1);

    domain::AlertRecord a2;
    a2.type = domain::AlertType::DISK_FULL;
    a2.stream_id = "cam_a";
    mgr.emit(a2);

    EXPECT_EQ(obs.alerts().size(), 2u);
}

TEST(AlertManager, NoDedupForDifferentStreams) {
    infrastructure::AlertManager mgr;
    mgr.set_dedup_cooldown_ms(10000);
    TestObserver obs;
    mgr.register_observer(&obs);

    domain::AlertRecord a1;
    a1.type = domain::AlertType::PIPELINE_STALL;
    a1.stream_id = "cam_a";
    mgr.emit(a1);

    domain::AlertRecord a2;
    a2.type = domain::AlertType::PIPELINE_STALL;
    a2.stream_id = "cam_b";
    mgr.emit(a2);

    EXPECT_EQ(obs.alerts().size(), 2u);
}
