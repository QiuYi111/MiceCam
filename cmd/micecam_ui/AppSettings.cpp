#include "AppSettings.h"

namespace micecam::ui {

AppSettings::AppSettings(QObject* parent)
    : QObject(parent) {}

int AppSettings::watchdogTimeout() const { return config_.watchdog_timeout_s(); }
void AppSettings::setWatchdogTimeout(int value) {
    config_.set_watchdog_timeout_s(value);
    emit watchdogTimeoutChanged();
}

double AppSettings::yellowDropThreshold() const { return config_.drop_rate_yellow_pct(); }
void AppSettings::setYellowDropThreshold(double value) {
    config_.set_drop_rate_yellow_pct(value);
    emit yellowDropThresholdChanged();
}

double AppSettings::redDropThreshold() const { return config_.drop_rate_red_pct(); }
void AppSettings::setRedDropThreshold(double value) {
    config_.set_drop_rate_red_pct(value);
    emit redDropThresholdChanged();
}

QString AppSettings::webhookUrl() const { return QString::fromStdString(config_.webhook_url()); }
void AppSettings::setWebhookUrl(const QString& value) {
    config_.set_webhook_url(value.toStdString());
    emit webhookUrlChanged();
}

int AppSettings::defaultBitrateKbps() const { return config_.default_bitrate_kbps(); }
void AppSettings::setDefaultBitrateKbps(int value) {
    config_.set_default_bitrate_kbps(value);
    emit defaultBitrateKbpsChanged();
}

QString AppSettings::outputDirectory() const { return QString::fromStdString(config_.output_dir()); }
void AppSettings::setOutputDirectory(const QString& value) {
    config_.set_output_dir(value.toStdString());
    emit outputDirectoryChanged();
}

QString AppSettings::logLevel() const { return QString::fromStdString(config_.log_level()); }
void AppSettings::setLogLevel(const QString& value) {
    config_.set_log_level(value.toStdString());
    emit logLevelChanged();
}

bool AppSettings::save() {
    return config_.save("micecam_config.json");
}

} // namespace micecam::ui
