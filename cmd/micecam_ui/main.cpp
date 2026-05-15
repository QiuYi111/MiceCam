#include <QQuickStyle>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include "AppController.h"

int main(int argc, char* argv[]) {
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);

    micecam::ui::AppController controller(micecam::ui::BackendMode::Production);
    controller.refreshCameras();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appController", &controller);
    engine.load(QUrl("qrc:/MiceCam/UI/qml/main.qml"));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
