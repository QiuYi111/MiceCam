import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MiceCam.Models
import "../theme"

Item {
    id: root

    signal cardFullscreen(string name, real fps, int drops, bool isRecording, int status)

    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            x: 24
            y: 16
            width: root.width - 48
            spacing: 12

            RowLayout {
                spacing: 12
                Layout.fillWidth: true
                Layout.preferredHeight: root.height / 2 - 22

                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_A"; fps: 29.97; drops: 0; status: 0; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_B"; fps: 29.97; drops: 0; status: 0; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                }
            }

            RowLayout {
                spacing: 12
                Layout.fillWidth: true
                Layout.preferredHeight: root.height / 2 - 22

                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_C"; fps: 29.97; drops: 0; status: 0; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_D"; fps: 18.45; drops: 152; status: 1; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "USB-1"; fps: 29.97; drops: 0; status: 0; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                }
            }
        }
    }
}
