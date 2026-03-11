#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QElapsedTimer>
#include <memory>
#include <vector>

class CameraInventoryModel;
class QMediaDevices;
class VideoFrameProvider; // Forward declare

// Forward declarations to keep Qt compile fast
namespace micecam {
    class IngestionPipeline;
    class ICameraBackend;
    class Decoder;
}

class PipelineController : public QObject {
    Q_OBJECT

    // QML Properties
    Q_PROPERTY(bool isRecording READ isRecording NOTIFY isRecordingChanged)
    Q_PROPERTY(QString sessionName READ getSessionName WRITE setSessionName NOTIFY sessionNameChanged)
    Q_PROPERTY(QString outputDir READ getOutputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool autoDecode READ getAutoDecode WRITE setAutoDecode NOTIFY autoDecodeChanged)
    Q_PROPERTY(QString sessionState READ getSessionState NOTIFY sessionStateChanged)
    Q_PROPERTY(QString statusHeadline READ getStatusHeadline NOTIFY sessionStateChanged)
    Q_PROPERTY(QString statusDetail READ getStatusDetail NOTIFY sessionStateChanged)
    Q_PROPERTY(QObject* cameraInventoryModel READ cameraInventoryModel CONSTANT)
    Q_PROPERTY(int selectedCameraIndex READ selectedCameraIndex WRITE setSelectedCameraIndex NOTIFY selectedCameraChanged)
    Q_PROPERTY(QStringList availableResolutions READ availableResolutions NOTIFY selectedCameraChanged)
    Q_PROPERTY(QVariantList availableFps READ availableFps NOTIFY selectedCameraChanged)
    Q_PROPERTY(QString selectedResolution READ selectedResolution WRITE setSelectedResolution NOTIFY captureConfigChanged)
    Q_PROPERTY(double requestedFps READ requestedFps WRITE setRequestedFps NOTIFY captureConfigChanged)
    Q_PROPERTY(bool canStartRecording READ canStartRecording NOTIFY readinessChanged)
    Q_PROPERTY(QString readinessMessage READ readinessMessage NOTIFY readinessChanged)
    Q_PROPERTY(QString sanitizedSessionName READ sanitizedSessionName NOTIFY readinessChanged)
    Q_PROPERTY(QString lastErrorMessage READ getLastErrorMessage NOTIFY errorStateChanged)
    Q_PROPERTY(QString lastSessionName READ getLastSessionName NOTIFY sessionArchiveChanged)
    Q_PROPERTY(QString decodedOutputDir READ getDecodedOutputDir NOTIFY sessionArchiveChanged)
    Q_PROPERTY(int elapsedSeconds READ getElapsedSeconds NOTIFY statsUpdated)
    Q_PROPERTY(QString recordingDurationText READ getRecordingDurationText NOTIFY statsUpdated)
    Q_PROPERTY(bool isDecoding READ isDecoding NOTIFY decodeStateChanged)
    Q_PROPERTY(bool hasAvailableCamera READ hasAvailableCamera NOTIFY cameraInventoryChanged)
    Q_PROPERTY(bool hasDroppedFramesWarning READ hasDroppedFramesWarning NOTIFY statsUpdated)
    Q_PROPERTY(QString resolvedSessionPath READ getResolvedSessionPath NOTIFY sessionArchiveChanged)
    Q_PROPERTY(QString resolvedExportPath READ getResolvedExportPath NOTIFY sessionArchiveChanged)

    // Log system
    Q_PROPERTY(QStringList logMessages READ logMessages NOTIFY logMessagesChanged)
    Q_PROPERTY(double decodeProgress READ getDecodeProgress NOTIFY decodeProgressChanged)


    // Stats for UI
    Q_PROPERTY(double currentFps READ getCurrentFps NOTIFY statsUpdated)
    Q_PROPERTY(uint64_t capturedFrames READ getCapturedFrames NOTIFY statsUpdated)
    Q_PROPERTY(uint64_t droppedFrames READ getDroppedFrames NOTIFY statsUpdated)
    Q_PROPERTY(double mbps READ getMbps NOTIFY statsUpdated)
    Q_PROPERTY(QString format READ getFormat NOTIFY captureStarted)

public:
    explicit PipelineController(QObject *parent = nullptr);
    ~PipelineController() override;

    // Property getters
    bool isRecording() const;
    QString getSessionName() const;
    QString getOutputDir() const;
    double getCurrentFps() const;
    uint64_t getCapturedFrames() const;
    uint64_t getDroppedFrames() const;
    double getMbps() const;
    QString getFormat() const;
    bool getAutoDecode() const;
    QString getSessionState() const;
    QString getStatusHeadline() const;
    QString getStatusDetail() const;
    QObject* cameraInventoryModel() const;
    int selectedCameraIndex() const;
    QStringList availableResolutions() const;
    QVariantList availableFps() const;
    QString selectedResolution() const;
    double requestedFps() const;
    bool canStartRecording() const;
    QString readinessMessage() const;
    QString sanitizedSessionName() const;
    QString getLastErrorMessage() const;
    QString getLastSessionName() const;
    QString getDecodedOutputDir() const;
    int getElapsedSeconds() const;
    QString getRecordingDurationText() const;
    bool isDecoding() const;
    bool hasAvailableCamera() const;
    bool hasDroppedFramesWarning() const;
    QString getResolvedSessionPath() const;
    QString getResolvedExportPath() const;
    QStringList logMessages() const;
    double getDecodeProgress() const;


    // Property setters
    void setSessionName(const QString& name);
    void setOutputDir(const QString& dir);
    void setAutoDecode(bool enable);
    void setSelectedCameraIndex(int index);
    void setSelectedResolution(const QString& resolution);
    void setRequestedFps(double fps);

    // Logging
    Q_INVOKABLE void log(const QString& message);


    // Q_INVOKABLE methods (callable from QML)
    Q_INVOKABLE void refreshCameraInventory();
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();

    // Attach QQuickImageProvider for preview
    void setVideoProvider(VideoFrameProvider* provider);

signals:
    void isRecordingChanged(bool isR);
    void sessionNameChanged(const QString& name);
    void outputDirChanged(const QString& dir);
    void statsUpdated();
    void captureStarted();
    void errorOccurred(const QString& errorMsg);
    void logMessagesChanged();
    void autoDecodeChanged(bool autoDecode);
    void decodeProgressChanged(double progress);
    void decodeStateChanged();
    void cameraInventoryChanged();
    void selectedCameraChanged();
    void captureConfigChanged();
    void sessionStateChanged();
    void errorStateChanged();
    void sessionArchiveChanged();
    void readinessChanged();


private:
    void refreshReadiness();
    QString currentBackendId() const;
    int currentDeviceIndex() const;
    void setSessionState(const QString& state, const QString& detail = QString());
    void setErrorState(const QString& errorMsg);
    void setDecodingState(bool decoding);

    bool m_isRecording = false;
    bool m_isDecoding = false;
    QString m_sessionName;
    QString m_outputDir = "recordings";
    QString m_sessionState = "idle";
    QString m_statusDetail = "Choose a camera and destination to prepare the next capture.";
    QString m_readinessMessage = "Connect a camera to begin.";
    QString m_sanitizedSessionName;
    QString m_lastErrorMessage;
    QString m_lastSessionName;
    QString m_decodedOutputDir;
    QString m_lastOutputDir;
    QElapsedTimer m_recordingTimer;
    CameraInventoryModel* m_cameraInventoryModel = nullptr;
    int m_selectedCameraIndex = -1;
    QString m_selectedResolution;
    double m_requestedFps = 30.0;
    bool m_canStartRecording = false;
    QMediaDevices* m_mediaDevices = nullptr;

    double m_currentFps = 0.0;
    uint64_t m_capturedFrames = 0;
    uint64_t m_droppedFrames = 0;
    double m_mbps = 0.0;
    QString m_format = "MJPEG";

    bool m_autoDecode = true;
    QStringList m_logMessages;
    double m_decodeProgress = 0.0;
    std::unique_ptr<micecam::Decoder> m_decoder;

    std::unique_ptr<micecam::ICameraBackend> m_cameraBackend;
    std::unique_ptr<micecam::IngestionPipeline> m_pipeline;
    int m_statsTimerId = 0;
    VideoFrameProvider* m_videoProvider = nullptr;

protected:
    void timerEvent(QTimerEvent *event) override;
};
