import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"

Item {
    id: root

    signal navigateToDetail(string pluginPath)

    property bool locked: appController.isRecording

    function reloadPlugins() {
        pluginListModel.clear()
        var plugins = appController.pluginList()
        for (var i = 0; i < plugins.length; i++) {
            pluginListModel.append(plugins[i])
        }
    }

    Component.onCompleted: reloadPlugins()

    Connections {
        target: appController
        function onPluginsChanged() { reloadPlugins() }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Text {
                text: "Plugin Management"
                font.family: Theme.fontPrimary
                font.pixelSize: 24
                font.weight: Font.Bold
                color: Theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Import Plugin"
                enabled: !root.locked
                opacity: root.locked ? 0.4 : 1.0

                background: Rectangle {
                    radius: 6
                    color: parent.enabled ? Theme.navyPrimary : Theme.bgTertiary
                }

                contentItem: Text {
                    text: parent.text
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: "#FFFFFF"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                ToolTip.visible: root.locked && hovered
                ToolTip.text: "Not available while recording"
                ToolTip.delay: 200

                onClicked: folderDialog.open()
            }
        }

        Text {
            text: root.locked ? "Plugin changes are disabled while recording" : ""
            font.family: Theme.fontPrimary
            font.pixelSize: 12
            color: Theme.statusAmber
            visible: root.locked
        }

        ListView {
            id: pluginListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2

            model: ListModel {
                id: pluginListModel
            }

            delegate: Rectangle {
                width: pluginListView.width
                height: 48
                radius: 6
                color: mouseArea.containsMouse ? Theme.bgSecondary : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 16

                    Text {
                        text: model.name
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: Theme.textPrimary
                        Layout.preferredWidth: 160
                    }

                    Rectangle {
                        Layout.preferredWidth: 64
                        height: 20
                        radius: 10
                        color: model.type === "bundled" ? Theme.navyTint : "#E8F5E9"

                        Text {
                            anchors.centerIn: parent
                            text: model.type
                            font.family: Theme.fontPrimary
                            font.pixelSize: 11
                            color: model.type === "bundled" ? Theme.navyPrimary : "#2E7D32"
                        }
                    }

                    Text {
                        text: "v" + model.version
                        font.family: Theme.fontMono
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        Layout.preferredWidth: 60
                    }

                    Switch {
                        checked: model.enabled
                        enabled: !root.locked
                        opacity: root.locked ? 0.4 : 1.0

                        ToolTip.visible: root.locked && hovered
                        ToolTip.text: "Not available while recording"
                        ToolTip.delay: 200

                        onToggled: {
                            appController.togglePlugin(model.path, checked)
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: model.status === "OK" ? Theme.statusGreen
                             : model.status === "Error" ? Theme.statusRed
                             : model.status === "Disabled" ? Theme.textTertiary
                             : Theme.statusAmber
                    }

                    Text {
                        text: model.status
                        font.family: Theme.fontPrimary
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        Layout.preferredWidth: 60
                    }

                    Text {
                        text: model.deviceCount + " device" + (model.deviceCount !== 1 ? "s" : "")
                        font.family: Theme.fontPrimary
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        Layout.preferredWidth: 80
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: "\u203A"
                        font.pixelSize: 18
                        color: Theme.textTertiary
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.navigateToDetail(model.path)
                }
            }
        }

        Text {
            text: pluginListModel.count === 0 ? "No plugins registered. Import a plugin directory to get started." : ""
            font.family: Theme.fontPrimary
            font.pixelSize: 14
            color: Theme.textTertiary
            visible: pluginListModel.count === 0
            Layout.alignment: Qt.AlignHCenter
        }
    }

    FolderDialog {
        id: folderDialog
        title: "Select Plugin Directory"
        onAccepted: {
            appController.importPlugin(selectedFolder.toString().replace("file://", ""))
        }
    }
}
