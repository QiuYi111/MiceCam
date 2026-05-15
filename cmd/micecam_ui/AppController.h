#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "AppAlertModel.h"
#include "AppCameraModel.h"
#include "AppSettings.h"
#include "infrastructure/CameraManager.h"
#include "infrastructure/MockCameraBackend.h"
#include "pipeline/RecordingPipeline.h"

namespace micecam::ui {

enum class BackendMode { Production, MockOnly };

class AppController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* cameraModel READ cameraModel CONSTANT)
    Q_PROPERTY(QAbstractListModel* alertModel READ alertModel CONSTANT)
    Q_PROPERTY(AppSettings* settings READ settings CONSTANT)
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QString recordButtonText READ recordButtonText NOTIFY recordButtonTextChanged)
    Q_PROPERTY(QString cameraCountText READ cameraCountText NOTIFY cameraCountTextChanged)
    Q_PROPERTY(QString elapsedText READ elapsedText NOTIFY elapsedTextChanged)
    Q_PROPERTY(QString totalFramesText READ totalFramesText NOTIFY totalFramesTextChanged)
    Q_PROPERTY(QString averageFpsText READ averageFpsText NOTIFY averageFpsTextChanged)
    Q_PROPERTY(QString bytesWrittenText READ bytesWrittenText NOTIFY bytesWrittenTextChanged)
    Q_PROPERTY(QString diskRemainingText READ diskRemainingText NOTIFY diskRemainingTextChanged)
    Q_PROPERTY(QString preflightMessage READ preflightMessage NOTIFY preflightMessageChanged)
    Q_PROPERTY(QString lastSessionId READ lastSessionId NOTIFY lastSessionIdChanged)

public:
    explicit AppController(BackendMode mode, QObject* parent = nullptr);

    QAbstractListModel* cameraModel() const;
    QAbstractListModel* alertModel() const;
    AppSettings* settings() const;
    bool isRecording() const;
    QString recordButtonText() const;
    QString cameraCountText() const;
    QString elapsedText() const;
    QString totalFramesText() const;
    QString averageFpsText() const;
    QString bytesWrittenText() const;
    QString diskRemainingText() const;
    QString preflightMessage() const;
    QString lastSessionId() const;

    Q_INVOKABLE void refreshCameras();
    Q_INVOKABLE bool startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE QVariantList preflightItems();
    Q_INVOKABLE QVariantMap cameraAt(int row);

    void setOutputDirectory(const QString& dir);

signals:
    void isRecordingChanged();
    void recordButtonTextChanged();
    void cameraCountTextChanged();
    void elapsedTextChanged();
    void totalFramesTextChanged();
    void averageFpsTextChanged();
    void bytesWrittenTextChanged();
    void diskRemainingTextChanged();
    void preflightMessageChanged();
    void lastSessionIdChanged();

private:
    struct ActiveStream {
        domain::StreamConfig config;
        std::unique_ptr<domain::CameraStream> stream;
    };

    BackendMode mode_;
    infrastructure::MockCameraBackend mock_backend_;
    infrastructure::CameraManager manager_;
    pipeline::RecordingPipeline pipeline_;
    AppCameraModel* camera_model_;
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

    std::atomic<bool> capture_running_{false};
    std::thread capture_thread_;
    std::vector<ActiveStream> active_streams_;

    void captureLoop();
    void stopCaptureLoop();
    void refreshLiveStatus();
};

} // namespace micecam::ui
