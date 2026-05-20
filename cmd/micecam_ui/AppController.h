#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include "AppAlertModel.h"
#include "AppCameraModel.h"
#include "AppSettings.h"
#include "CameraSourceModel.h"
#include "infrastructure/CameraManager.h"
#include "infrastructure/PluginRegistryService.h"
#include "pipeline/RecordingPipeline.h"

namespace micecam::ui {

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* cameraModel READ cameraModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* sourceModel READ sourceModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* alertModel READ alertModel CONSTANT)
    Q_PROPERTY(AppSettings* settings READ settings CONSTANT)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QString recordButtonText READ recordButtonText NOTIFY recordButtonTextChanged)
    Q_PROPERTY(bool canStartRecording READ canStartRecording NOTIFY canStartRecordingChanged)
    Q_PROPERTY(int cameraCount READ cameraCount NOTIFY cameraCountChanged)
    Q_PROPERTY(QString cameraCountText READ cameraCountText NOTIFY cameraCountTextChanged)
    Q_PROPERTY(QString elapsedText READ elapsedText NOTIFY elapsedTextChanged)
    Q_PROPERTY(QString totalFramesText READ totalFramesText NOTIFY totalFramesTextChanged)
    Q_PROPERTY(QString averageFpsText READ averageFpsText NOTIFY averageFpsTextChanged)
    Q_PROPERTY(QString bytesWrittenText READ bytesWrittenText NOTIFY bytesWrittenTextChanged)
    Q_PROPERTY(QString diskRemainingText READ diskRemainingText NOTIFY diskRemainingTextChanged)
    Q_PROPERTY(QString preflightMessage READ preflightMessage NOTIFY preflightMessageChanged)
    Q_PROPERTY(QString lastSessionId READ lastSessionId NOTIFY lastSessionIdChanged)
    Q_PROPERTY(QString currentEncoderName READ currentEncoderName NOTIFY encoderNameChanged)
    Q_PROPERTY(QString currentBitrate READ currentBitrate NOTIFY bitrateChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString buildDate READ buildDate CONSTANT)

    Q_PROPERTY(QStringList recentLogEntries READ recentLogEntries NOTIFY logEntriesChanged)

public:
    explicit AppController(QObject* parent = nullptr);

    QAbstractListModel* cameraModel() const;
    QAbstractListModel* sourceModel() const;
    QAbstractListModel* alertModel() const;
    AppSettings* settings() const;
    bool isRecording() const;
    QString recordButtonText() const;
    bool canStartRecording() const;
    int cameraCount() const;
    QString cameraCountText() const;
    QString elapsedText() const;
    QString totalFramesText() const;
    QString averageFpsText() const;
    QString bytesWrittenText() const;
    QString diskRemainingText() const;
    QString preflightMessage() const;
    QString lastSessionId() const;
    QString currentEncoderName() const;
    QString currentBitrate() const;

    QString appVersion() const;
    QString buildDate() const;
    QStringList recentLogEntries() const;

    Q_INVOKABLE void refreshCameras();
    Q_INVOKABLE bool startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE QVariantList preflightItems();
    Q_INVOKABLE QVariantMap cameraAt(int row);

    Q_INVOKABLE QVariantList pluginList();
    Q_INVOKABLE bool importPlugin(const QString& dirPath);
    Q_INVOKABLE void togglePlugin(const QString& pluginPath, bool enabled);
    Q_INVOKABLE bool removePlugin(const QString& pluginPath);
    Q_INVOKABLE QVariantMap getPluginDetail(const QString& pluginPath);

    void setOutputDirectory(const QString& dir);

    signals:
    void isRecordingChanged();
    void recordButtonTextChanged();
    void canStartRecordingChanged();
    void cameraCountChanged();
    void cameraCountTextChanged();
    void elapsedTextChanged();
    void totalFramesTextChanged();
    void averageFpsTextChanged();
    void bytesWrittenTextChanged();
    void diskRemainingTextChanged();
    void preflightMessageChanged();
    void lastSessionIdChanged();
    void encoderNameChanged();
    void bitrateChanged();
    void logEntriesChanged();
    void deviceDisconnected(const QString& deviceName);
    void pluginCrashAlert(const QString& pluginId);
    void pluginsChanged();

public slots:
    void handlePluginCrash(const std::string& pluginId);
    void handleDeviceDisconnect(const std::string& streamId, const std::string& deviceName);

private:
    struct ActiveStream {
        domain::StreamConfig config;
        std::unique_ptr<domain::CameraStream> stream;
    };

    infrastructure::PluginRegistryService plugin_registry_;
    infrastructure::CameraManager manager_;
    pipeline::RecordingPipeline pipeline_;
    AppCameraModel* camera_model_;
    CameraSourceModel* source_model_;
    AppAlertModel* alert_model_;
    AppSettings* settings_;

    bool recording_ = false;
    QString output_dir_;
    QString session_id_;
    uint64_t total_frames_ = 0;
    double average_fps_ = 0.0;
    uint64_t bytes_written_ = 0;
    QString disk_remaining_;
    QString preflight_message_;
    QString current_encoder_name_ = QStringLiteral("—");
    QString current_bitrate_ = QStringLiteral("—");
    QString app_version_ = QStringLiteral("0.0.0");
    QString build_date_ = QStringLiteral("unknown");
    std::chrono::steady_clock::time_point session_start_;

    std::atomic<bool> capture_running_{false};
    std::thread capture_thread_;
    std::vector<ActiveStream> active_streams_;
    QStringList log_entries_;

    QTimer* metrics_timer_ = nullptr;
    std::unordered_map<std::string, uint64_t> stream_frame_counts_;
    std::unordered_map<std::string, uint64_t> stream_drop_counts_;
    std::chrono::steady_clock::time_point last_metrics_push_;

    void captureLoop();
    void stopCaptureLoop();
    void refreshLiveStatus();
    void pushLiveMetrics();
    void setupCrashAlertHandler();
};

} // namespace micecam::ui
