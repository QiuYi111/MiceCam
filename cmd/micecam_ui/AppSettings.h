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

    Q_INVOKABLE bool save();

signals:
    void watchdogTimeoutChanged();
    void yellowDropThresholdChanged();
    void redDropThresholdChanged();
    void webhookUrlChanged();
    void defaultBitrateKbpsChanged();
    void outputDirectoryChanged();
    void logLevelChanged();

private:
    infrastructure::ConfigLoader config_;
};

} // namespace micecam::ui
