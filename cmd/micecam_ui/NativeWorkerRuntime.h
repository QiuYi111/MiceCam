#pragma once

#include <QObject>
#include <QJsonObject>

class QFile;
class QSocketNotifier;

namespace micecam {
class ICameraBackend;
class IngestionPipeline;
class Decoder;
}

namespace micecam_ui {

class NativeWorkerRuntime : public QObject {
    Q_OBJECT

public:
    explicit NativeWorkerRuntime(QObject* parent = nullptr);
    ~NativeWorkerRuntime() override;

    void start();

private:
    void processIncomingData();
    void processCommand(const QByteArray& line);
    void startRecording(const QJsonObject& command);
    void stopRecording();
    void requestShutdown();
    void publishStatus(const QString& state, const QString& detail = QString(), double decodeProgress = -1.0);
    void publishActivity(const QString& severity, const QString& category, const QString& message, const QString& relatedPath = QString());
    void finishCompletedState();

    QFile* m_stdinFile = nullptr;
    QSocketNotifier* m_stdinNotifier = nullptr;
    QByteArray m_inputBuffer;
    std::unique_ptr<micecam::Decoder> m_decoder;
    std::unique_ptr<micecam::ICameraBackend> m_cameraBackend;
    std::unique_ptr<micecam::IngestionPipeline> m_pipeline;
    int m_statsTimerId = 0;
    QString m_outputDir;
    QString m_sessionName;
    QString m_exportPath;
    QString m_backendId;
    bool m_autoDecode = true;
    bool m_isRecording = false;
    bool m_isDecoding = false;
    bool m_shutdownRequested = false;
    uint64_t m_lastCapturedFrames = 0;
    double m_lastCurrentFps = 0.0;

protected:
    void timerEvent(QTimerEvent* event) override;
};

}  // namespace micecam_ui
