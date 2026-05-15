import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
        id: gridScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Flow {
            width: gridScroll.availableWidth
            spacing: 12
            padding: 16

            Repeater {
                model: appController.cameraModel
                delegate: CameraCard {
                    width: index < 2 ? (parent.width - 12) / 2 : (parent.width - 24) / 3
                    height: index < 2 ? root.height / 2 - 22 : root.height / 2 - 22
                    cameraName: model.name
                    fps: model.fps
                    drops: model.dropCount
                    status: model.status
                    isRecording: model.isRecording
                    onContextFullscreen: root.cardFullscreen(model.name, model.fps, model.dropCount, model.isRecording, model.status)
                    onContextMenuRequested: function(gx, gy) { root.showContextMenu(model.name, model.fps, model.dropCount, model.isRecording, model.status, gx, gy) }
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
