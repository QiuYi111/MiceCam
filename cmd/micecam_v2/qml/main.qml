import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 1280
    height: 800
    title: "MiceCam v2"

    Rectangle {
        anchors.fill: parent
        color: "#F2F2F7"

        Text {
            anchors.centerIn: parent
            text: "MiceCam v2\nFoundation Ready"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            color: "#1C1C1E"
        }
    }
}
