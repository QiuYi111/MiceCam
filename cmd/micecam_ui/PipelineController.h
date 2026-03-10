#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QStringList>
#include <QProcess>
#include <memory>
#include <vector>

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
    QStringList logMessages() const;
    double getDecodeProgress() const;


    // Property setters
    void setSessionName(const QString& name);
    void setOutputDir(const QString& dir);
    void setAutoDecode(bool enable);

    // Logging
    Q_INVOKABLE void log(const QString& message);


    // Q_INVOKABLE methods (callable from QML)
    Q_INVOKABLE QVariantList getAvailableCameras();
    Q_INVOKABLE QVariantList getAvailableResolutions(const QString& backend);
    Q_INVOKABLE void startRecording(const QString& backend, int type, int width, int height, double fps);
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


private:
    void updateStatsTimer();

    bool m_isRecording = false;
    QString m_sessionName;
    QString m_outputDir = "recordings";

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
