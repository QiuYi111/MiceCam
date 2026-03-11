#include <QCoreApplication>
#include <QGuiApplication>
#include <QSocketNotifier>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QCommandLineParser>
#include <QtQuickControls2/QQuickStyle>
#include <csignal>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif
#include "NativeWorkerRuntime.h"
#include "PipelineController.h"
#include "VideoFrameProvider.h"

namespace {

#ifdef Q_OS_UNIX
int gSignalPipe[2] = {-1, -1};

void forwardUnixSignal(int signalNumber) {
    const unsigned char signalByte = static_cast<unsigned char>(signalNumber);
    if (gSignalPipe[1] >= 0) {
        ::write(gSignalPipe[1], &signalByte, sizeof(signalByte));
    }
}
#endif

}  // namespace

int main(int argc, char *argv[]) {
    bool workerMode = false;
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) == "--worker") {
            workerMode = true;
            break;
        }
    }

    if (workerMode) {
        QCoreApplication app(argc, argv);
        app.setOrganizationName("MiceCam");
        app.setOrganizationDomain("micecam.local");
        app.setApplicationName("MiceCam Worker");
        app.setApplicationVersion("1.0.0");

        micecam_ui::NativeWorkerRuntime worker;
        worker.start();
        return app.exec();
    }

    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);

    app.setOrganizationName("MiceCam");
    app.setOrganizationDomain("micecam.local");
    app.setApplicationName("MiceCam");
    app.setApplicationVersion("1.0.0");

    QQmlApplicationEngine engine;

    // Register backend controller
    auto* pipelineController = new PipelineController(&app);
    auto* videoProvider = new VideoFrameProvider();

    QObject::connect(&app, &QCoreApplication::aboutToQuit, pipelineController, &PipelineController::shutdownForExit);

    QSocketNotifier* signalNotifier = nullptr;
#ifdef Q_OS_UNIX
    if (::pipe(gSignalPipe) == 0) {
        std::signal(SIGINT, forwardUnixSignal);
        std::signal(SIGTERM, forwardUnixSignal);
        signalNotifier = new QSocketNotifier(gSignalPipe[0], QSocketNotifier::Read, &app);
        auto* notifier = signalNotifier;
        QObject::connect(notifier, &QSocketNotifier::activated, &app, [&app, notifier, pipelineController]() {
            notifier->setEnabled(false);
            unsigned char signalByte = 0;
            ::read(gSignalPipe[0], &signalByte, sizeof(signalByte));
            pipelineController->shutdownForExit();
            app.quit();
        });
    }
#endif
    Q_UNUSED(signalNotifier);

    // Allow QML to read frames from image://live_camera/feed
    engine.addImageProvider("live_camera", videoProvider);

    // Pass the provider to the controller so it can attach to the C++ pipeline
    pipelineController->setVideoProvider(videoProvider);

    // Make PipelineController available globally in QML as 'pipeline'
    engine.rootContext()->setContextProperty("pipeline", pipelineController);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
