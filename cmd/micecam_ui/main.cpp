#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QCommandLineParser>
#include <iostream>
#include "PipelineController.h"
#include "VideoFrameProvider.h"

int main(int argc, char *argv[]) {
    // Set up high DPI scaling
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QGuiApplication app(argc, argv);

    app.setOrganizationName("MiceCam");
    app.setOrganizationDomain("micecam.local");
    app.setApplicationName("MiceCam");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("MiceCam High Performance Camera Suite");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption workerOption("worker", "Run as a headless recording worker process.");
    parser.addOption(workerOption);

    parser.process(app);

    if (parser.isSet(workerOption)) {
        // TODO: Implement headless worker entry point if process isolation is required
        std::cout << "MiceCam: Running in worker mode (Headless)" << std::endl;
        return 0; // Worker logic would go here
    }

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

    std::cout << "Starting MiceCam UI..." << std::endl;
    engine.load(url);

    return app.exec();
}
