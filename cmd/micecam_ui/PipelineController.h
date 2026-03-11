#pragma once

#include <QObject>
#include <QElapsedTimer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <memory>

class CameraInventoryModel;
class QMediaDevices;
class VideoFrameProvider;

namespace micecam_ui {
class RecordingSupervisorService;
}

class PipelineController : public QObject {
    Q_OBJECT

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
    Q_PROPERTY(bool previewAvailable READ previewAvailable NOTIFY sessionStateChanged)
    Q_PROPERTY(QString previewMode READ previewMode NOTIFY sessionStateChanged)
    Q_PROPERTY(QString previewDetail READ previewDetail NOTIFY sessionStateChanged)
    Q_PROPERTY(QStringList logMessages READ logMessages NOTIFY logMessagesChanged)
    Q_PROPERTY(double decodeProgress READ getDecodeProgress NOTIFY decodeProgressChanged)
    Q_PROPERTY(double currentFps READ getCurrentFps NOTIFY statsUpdated)
    Q_PROPERTY(uint64_t capturedFrames READ getCapturedFrames NOTIFY statsUpdated)
    Q_PROPERTY(uint64_t droppedFrames READ getDroppedFrames NOTIFY statsUpdated)
    Q_PROPERTY(double mbps READ getMbps NOTIFY statsUpdated)
    Q_PROPERTY(QString format READ getFormat NOTIFY captureConfigChanged)
    Q_PROPERTY(QObject* activityModel READ activityModel CONSTANT)

public:
    explicit PipelineController(QObject* parent = nullptr);
    ~PipelineController() override;

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
    QObject* activityModel() const;
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
    bool previewAvailable() const;
    QString previewMode() const;
    QString previewDetail() const;
    QStringList logMessages() const;
    double getDecodeProgress() const;

    void setSessionName(const QString& name);
    void setOutputDir(const QString& dir);
    void setAutoDecode(bool enable);
    void setSelectedCameraIndex(int index);
    void setSelectedResolution(const QString& resolution);
    void setRequestedFps(double fps);

    Q_INVOKABLE void refreshCameraInventory();
    Q_INVOKABLE void startRecording();
    Q_INVOKABLE void stopRecording();
    Q_INVOKABLE bool openResolvedOutput();
    Q_INVOKABLE bool requestAppClose();

    void setVideoProvider(VideoFrameProvider* provider);

signals:
    void isRecordingChanged(bool isR);
    void sessionNameChanged(const QString& name);
    void outputDirChanged(const QString& dir);
    void statsUpdated();
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
    void syncFromSupervisor();
    void rebuildLogMessages();

    QString m_sessionName;
    QString m_outputDir = "recordings";
    QString m_readinessMessage = "Connect a camera to begin.";
    QString m_sanitizedSessionName;
    QString m_lastSessionName;
    QElapsedTimer m_recordingTimer;
    CameraInventoryModel* m_cameraInventoryModel = nullptr;
    int m_selectedCameraIndex = -1;
    QString m_selectedResolution;
    double m_requestedFps = 30.0;
    bool m_canStartRecording = false;
    bool m_autoDecode = true;
    QStringList m_logMessages;
    QMediaDevices* m_mediaDevices = nullptr;
    std::unique_ptr<micecam_ui::RecordingSupervisorService> m_supervisor;
    int m_elapsedTimerId = 0;
    VideoFrameProvider* m_videoProvider = nullptr;

protected:
    void timerEvent(QTimerEvent* event) override;
};
