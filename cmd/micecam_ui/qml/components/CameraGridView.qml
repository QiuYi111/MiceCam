import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    signal cardFullscreen(string name, real fps, int drops, bool isRecording, int status, var resOpts, var fpsOpts, var fmtOpts)
    signal cardConfigure(string name, real fps, int drops, bool isRecording, int status, var resOpts, var fpsOpts, var fmtOpts)

    property string menuTargetName: ""
    property real menuTargetFps: 0
    property int menuTargetDrops: 0
    property bool menuTargetRecording: false
    property int menuTargetStatus: 0
    readonly property bool empty: appController.cameraCount === 0 && sourceRepeater.count === 0

    ScrollView {
        id: gridScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Column {
            width: gridScroll.availableWidth
            spacing: 12
            padding: 16

            Repeater {
                id: sourceRepeater
                model: appController.sourceModel

                delegate: Column {
                    width: parent.width - 32
                    spacing: 8
                    property var sourceDevices: model.devices || []

                    RowLayout {
                        width: parent.width
                        height: 28
                        spacing: 8

                        Text {
                            text: model.sourceName
                            font.family: Theme.fontPrimary
                            font.pixelSize: 15
                            font.weight: Font.DemiBold
                            color: Theme.textPrimary
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Rectangle {
                            height: 22
                            width: apiText.implicitWidth + 16
                            radius: 11
                            color: Theme.bgSecondary
                            visible: model.pluginApiVersion > 0
                            Text {
                                id: apiText
                                anchors.centerIn: parent
                                text: "API v" + model.pluginApiVersion
                                font.family: Theme.fontPrimary
                                font.pixelSize: 11
                                color: Theme.textSecondary
                            }
                        }

                        Rectangle {
                            height: 22
                            width: countText.implicitWidth + 16
                            radius: 11
                            color: Theme.bgSecondary
                            Text {
                                id: countText
                                anchors.centerIn: parent
                                text: model.deviceCount + " devices"
                                font.family: Theme.fontPrimary
                                font.pixelSize: 11
                                color: Theme.textSecondary
                            }
                        }

                        Rectangle {
                            width: 8; height: 8; radius: 4
                            color: model.diagnostics === 0 ? Theme.statusGreen
                                 : model.diagnostics === 2 ? Theme.textTertiary
                                 : model.diagnostics === 3 ? Theme.statusRed
                                 : Theme.statusAmber
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: 12
                        visible: model.deviceCount > 0

                        Repeater {
                            model: sourceDevices
                            delegate: CameraCard {
                                width: parent.width > 760 ? (parent.width - 12) / 2 : parent.width
                                height: Math.max(220, root.height / 2 - 42)
                                cameraName: modelData.name || modelData.displayName
                                fps: modelData.fps || 0
                                drops: modelData.dropCount || 0
                                status: modelData.statusCode || 0
                                isRecording: modelData.isRecording || false
                                elapsedText: appController.elapsedText
                                onContextFullscreen: root.cardFullscreen(cameraName, fps, drops, isRecording, status, [], [], [])
                                onContextMenuRequested: function(gx, gy) {
                                    root.showContextMenu(cameraName, fps, drops, isRecording, status, gx, gy)
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width
                        height: 48
                        radius: 8
                        color: Theme.bgSecondary
                        border.color: Theme.divider
                        visible: model.deviceCount === 0

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 10

                            AppIcon {
                                name: model.diagnostics === 3 ? "warning" : "camera"
                                size: 16
                                color: model.diagnostics === 3 ? Theme.statusRed : Theme.textSecondary
                            }

                            Text {
                                text: model.sourceName
                                font.family: Theme.fontPrimary
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: Theme.textPrimary
                            }

                            Text {
                                text: model.diagnosticsMessage || model.statusLabel || "No devices"
                                font.family: Theme.fontPrimary
                                font.pixelSize: 13
                                color: Theme.textSecondary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.restartRequired ? "restart required" : ""
                                font.family: Theme.fontPrimary
                                font.pixelSize: 12
                                color: Theme.statusAmber
                                visible: model.restartRequired
                            }
                        }
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        visible: root.empty

        ColumnLayout {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 360)
            spacing: 10

            AppIcon {
                name: "camera"
                size: 32
                color: Theme.statusAmber
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "No cameras detected"
                font.family: Theme.fontPrimary
                font.pixelSize: 18
                font.weight: Font.DemiBold
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }

            Text {
                text: appController.preflightMessage
                font.family: Theme.fontPrimary
                font.pixelSize: 13
                color: Theme.textSecondary
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
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
