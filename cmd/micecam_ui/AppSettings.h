#pragma once

#include <QObject>
#include <QString>

#include "infrastructure/ConfigLoader.h"

namespace micecam::ui {

class AppSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(int watchdogTimeout READ watchdogTimeout WRITE setWatchdogTimeout NOTIFY watchdogTimeoutChanged)
    Q_PROPERTY(double yellowDropThreshold READ yellowDropThreshold WRITE setYellowDropThreshold NOTIFY yellowDropThresholdChanged)
    Q_PROPERTY(double redDropThreshold READ redDropThreshold WRITE setRedDropThreshold NOTIFY redDropThresholdChanged)
    Q_PROPERTY(QString webhookUrl READ webhookUrl WRITE setWebhookUrl NOTIFY webhookUrlChanged)
    Q_PROPERTY(int defaultBitrateKbps READ defaultBitrateKbps WRITE setDefaultBitrateKbps NOTIFY defaultBitrateKbpsChanged)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY outputDirectoryChanged)
    Q_PROPERTY(QString logLevel READ logLevel WRITE setLogLevel NOTIFY logLevelChanged)
    Q_PROPERTY(int keyframeInterval READ keyframeInterval WRITE setKeyframeInterval NOTIFY keyframeIntervalChanged)
    Q_PROPERTY(QString encoderPreset READ encoderPreset WRITE setEncoderPreset NOTIFY encoderPresetChanged)
    Q_PROPERTY(bool hardwareAcceleration READ hardwareAcceleration WRITE setHardwareAcceleration NOTIFY hardwareAccelerationChanged)
    Q_PROPERTY(QString previewQuality READ previewQuality WRITE setPreviewQuality NOTIFY previewQualityChanged)
    Q_PROPERTY(bool desktopNotifications READ desktopNotifications WRITE setDesktopNotifications NOTIFY desktopNotificationsChanged)
    Q_PROPERTY(bool soundAlerts READ soundAlerts WRITE setSoundAlerts NOTIFY soundAlertsChanged)
    Q_PROPERTY(bool verboseDiagnostics READ verboseDiagnostics WRITE setVerboseDiagnostics NOTIFY verboseDiagnosticsChanged)
    Q_PROPERTY(bool createSubfolder READ createSubfolder WRITE setCreateSubfolder NOTIFY createSubfolderChanged)
    Q_PROPERTY(QString folderNamePrefix READ folderNamePrefix WRITE setFolderNamePrefix NOTIFY folderNamePrefixChanged)
    Q_PROPERTY(QString namingPattern READ namingPattern WRITE setNamingPattern NOTIFY namingPatternChanged)
    Q_PROPERTY(QString containerFormat READ containerFormat WRITE setContainerFormat NOTIFY containerFormatChanged)
    Q_PROPERTY(int maxFileSizeGB READ maxFileSizeGB WRITE setMaxFileSizeGB NOTIFY maxFileSizeGBChanged)

public:
    explicit AppSettings(QObject* parent = nullptr);

    int watchdogTimeout() const;
    void setWatchdogTimeout(int value);
    double yellowDropThreshold() const;
    void setYellowDropThreshold(double value);
    double redDropThreshold() const;
    void setRedDropThreshold(double value);
    QString webhookUrl() const;
    void setWebhookUrl(const QString& value);
    int defaultBitrateKbps() const;
    void setDefaultBitrateKbps(int value);
    QString outputDirectory() const;
    void setOutputDirectory(const QString& value);
    QString logLevel() const;
    void setLogLevel(const QString& value);

    int keyframeInterval() const;
    void setKeyframeInterval(int value);
    QString encoderPreset() const;
    void setEncoderPreset(const QString& value);
    bool hardwareAcceleration() const;
    void setHardwareAcceleration(bool value);
    QString previewQuality() const;
    void setPreviewQuality(const QString& value);
    bool desktopNotifications() const;
    void setDesktopNotifications(bool value);
    bool soundAlerts() const;
    void setSoundAlerts(bool value);
    bool verboseDiagnostics() const;
    void setVerboseDiagnostics(bool value);
    bool createSubfolder() const;
    void setCreateSubfolder(bool value);
    QString folderNamePrefix() const;
    void setFolderNamePrefix(const QString& value);
    QString namingPattern() const;
    void setNamingPattern(const QString& value);
    QString containerFormat() const;
    void setContainerFormat(const QString& value);
    int maxFileSizeGB() const;
    void setMaxFileSizeGB(int value);

    Q_INVOKABLE bool save();

signals:
    void watchdogTimeoutChanged();
    void yellowDropThresholdChanged();
    void redDropThresholdChanged();
    void webhookUrlChanged();
    void defaultBitrateKbpsChanged();
    void outputDirectoryChanged();
    void logLevelChanged();
    void keyframeIntervalChanged();
    void encoderPresetChanged();
    void hardwareAccelerationChanged();
    void previewQualityChanged();
    void desktopNotificationsChanged();
    void soundAlertsChanged();
    void verboseDiagnosticsChanged();
    void createSubfolderChanged();
    void folderNamePrefixChanged();
    void namingPatternChanged();
    void containerFormatChanged();
    void maxFileSizeGBChanged();

private:
    infrastructure::ConfigLoader config_;
};

} // namespace micecam::ui
