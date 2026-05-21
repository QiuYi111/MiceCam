#include "AppSettings.h"

namespace micecam::ui {

AppSettings::AppSettings(QObject* parent)
    : QObject(parent) {
    config_.load("micecam_config.json");
}

int AppSettings::watchdogTimeout() const { return config_.watchdog_timeout_s(); }
void AppSettings::setWatchdogTimeout(int value) {
    config_.set_watchdog_timeout_s(value);
    emit watchdogTimeoutChanged();
    config_.save("micecam_config.json");
}

double AppSettings::yellowDropThreshold() const { return config_.drop_rate_yellow_pct(); }
void AppSettings::setYellowDropThreshold(double value) {
    config_.set_drop_rate_yellow_pct(value);
    emit yellowDropThresholdChanged();
    config_.save("micecam_config.json");
}

double AppSettings::redDropThreshold() const { return config_.drop_rate_red_pct(); }
void AppSettings::setRedDropThreshold(double value) {
    config_.set_drop_rate_red_pct(value);
    emit redDropThresholdChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::webhookUrl() const { return QString::fromStdString(config_.webhook_url()); }
void AppSettings::setWebhookUrl(const QString& value) {
    config_.set_webhook_url(value.toStdString());
    emit webhookUrlChanged();
    config_.save("micecam_config.json");
}

int AppSettings::defaultBitrateKbps() const { return config_.default_bitrate_kbps(); }
void AppSettings::setDefaultBitrateKbps(int value) {
    config_.set_default_bitrate_kbps(value);
    emit defaultBitrateKbpsChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::outputDirectory() const { return QString::fromStdString(config_.output_dir()); }
void AppSettings::setOutputDirectory(const QString& value) {
    config_.set_output_dir(value.toStdString());
    emit outputDirectoryChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::logLevel() const { return QString::fromStdString(config_.log_level()); }
void AppSettings::setLogLevel(const QString& value) {
    config_.set_log_level(value.toStdString());
    emit logLevelChanged();
    config_.save("micecam_config.json");
}

int AppSettings::keyframeInterval() const { return config_.keyframe_interval(); }
void AppSettings::setKeyframeInterval(int value) {
    config_.set_keyframe_interval(value);
    emit keyframeIntervalChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::encoderPreset() const { return QString::fromStdString(config_.encoder_preset()); }
void AppSettings::setEncoderPreset(const QString& value) {
    config_.set_encoder_preset(value.toStdString());
    emit encoderPresetChanged();
    config_.save("micecam_config.json");
}

bool AppSettings::hardwareAcceleration() const { return config_.hardware_acceleration(); }
void AppSettings::setHardwareAcceleration(bool value) {
    config_.set_hardware_acceleration(value);
    emit hardwareAccelerationChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::previewQuality() const { return QString::fromStdString(config_.preview_quality()); }
void AppSettings::setPreviewQuality(const QString& value) {
    config_.set_preview_quality(value.toStdString());
    emit previewQualityChanged();
    config_.save("micecam_config.json");
}

bool AppSettings::desktopNotifications() const { return config_.desktop_notifications(); }
void AppSettings::setDesktopNotifications(bool value) {
    config_.set_desktop_notifications(value);
    emit desktopNotificationsChanged();
    config_.save("micecam_config.json");
}

bool AppSettings::soundAlerts() const { return config_.sound_alerts(); }
void AppSettings::setSoundAlerts(bool value) {
    config_.set_sound_alerts(value);
    emit soundAlertsChanged();
    config_.save("micecam_config.json");
}

bool AppSettings::verboseDiagnostics() const { return config_.verbose_diagnostics(); }
void AppSettings::setVerboseDiagnostics(bool value) {
    config_.set_verbose_diagnostics(value);
    emit verboseDiagnosticsChanged();
    config_.save("micecam_config.json");
}

bool AppSettings::createSubfolder() const { return config_.create_subfolder_per_session(); }
void AppSettings::setCreateSubfolder(bool value) {
    config_.set_create_subfolder_per_session(value);
    emit createSubfolderChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::folderNamePrefix() const { return QString::fromStdString(config_.folder_name_prefix()); }
void AppSettings::setFolderNamePrefix(const QString& value) {
    config_.set_folder_name_prefix(value.toStdString());
    emit folderNamePrefixChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::namingPattern() const { return QString::fromStdString(config_.naming_pattern()); }
void AppSettings::setNamingPattern(const QString& value) {
    config_.set_naming_pattern(value.toStdString());
    emit namingPatternChanged();
    config_.save("micecam_config.json");
}

QString AppSettings::containerFormat() const { return QString::fromStdString(config_.container_format()); }
void AppSettings::setContainerFormat(const QString& value) {
    config_.set_container_format(value.toStdString());
    emit containerFormatChanged();
    config_.save("micecam_config.json");
}

int AppSettings::maxFileSizeGB() const { return config_.max_file_size_gb(); }
void AppSettings::setMaxFileSizeGB(int value) {
    config_.set_max_file_size_gb(value);
    emit maxFileSizeGBChanged();
    config_.save("micecam_config.json");
}

bool AppSettings::save() {
    return config_.save("micecam_config.json");
}

} // namespace micecam::ui
