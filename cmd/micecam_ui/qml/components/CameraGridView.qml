import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MiceCam.Models
import "../theme"

Item {
    id: root

    signal cardFullscreen(string name, real fps, int drops, bool isRecording, int status)
    signal cardConfigure(string name, real fps, int drops, bool isRecording, int status)

    property string menuTargetName: ""
    property real menuTargetFps: 0
    property int menuTargetDrops: 0
    property bool menuTargetRecording: false
    property int menuTargetStatus: 0

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
                    onContextMenuRequested: function(gx, gy) { root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy) }
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_B"; fps: 29.97; drops: 0; status: 0; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                    onContextMenuRequested: function(gx, gy) { root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy) }
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
                    onContextMenuRequested: function(gx, gy) { root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy) }
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_D"; fps: 18.45; drops: 152; status: 1; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                    onContextMenuRequested: function(gx, gy) { root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy) }
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "USB-1"; fps: 29.97; drops: 0; status: 0; isRecording: true
                    onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status)
                    onContextMenuRequested: function(gx, gy) { root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy) }
                }
            }
        }
    }

    CameraContextMenu {
        id: sharedContextMenu
        cameraName: root.menuTargetName
        onConfigureClicked: {
            root.cardConfigure(root.menuTargetName, root.menuTargetFps, root.menuTargetDrops, root.menuTargetRecording, root.menuTargetStatus)
        }
        onFullscreenClicked: {
            root.cardFullscreen(root.menuTargetName, root.menuTargetFps, root.menuTargetDrops, root.menuTargetRecording, root.menuTargetStatus)
        }
        onRemoveClicked: {
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        z: 500
        onClicked: {
            sharedContextMenu.hide()
        }
        visible: sharedContextMenu.menuVisible
    }

    function showContextMenu(name, fps, drops, isRecording, status, gx, gy) {
        root.menuTargetName = name
        root.menuTargetFps = fps
        root.menuTargetDrops = drops
        root.menuTargetRecording = isRecording
        root.menuTargetStatus = status

        var menuW = 200
        var menuH = 140
        var stackPos = root.mapFromItem(null, gx, gy)
        var cx = Math.max(0, Math.min(stackPos.x, root.width - menuW - 8))
        var cy = Math.max(0, Math.min(stackPos.y, root.height - menuH - 8))

        sharedContextMenu.show(cx, cy)
    }
}
