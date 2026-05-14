#include <QQuickStyle>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include "MockCameraModel.h"

int main(int argc, char* argv[]) {
    QQuickStyle::setStyle("Basic");
    QGuiApplication app(argc, argv);

    qmlRegisterType<MockCameraModel>("MiceCam.Models", 1, 0, "CameraModel");

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/MiceCam/UI/qml/main.qml"));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
