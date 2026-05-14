#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QCommandLineParser>
#include <QtQuickControls2/QQuickStyle>
#include "NativeWorkerRuntime.h"
#include "PipelineController.h"
#include "VideoFrameProvider.h"

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
