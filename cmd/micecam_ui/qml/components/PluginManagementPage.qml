import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"

Item {
    id: root

    signal navigateToDetail(string pluginPath)

    property bool locked: appController.isRecording
    property string importError: ""

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
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 3
                Text {
                    text: "Plugin Management"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 24
                    font.weight: Font.Bold
                    color: Theme.textPrimary
                }
                Text {
                    text: "Enable, disable, and configure installed plugins."
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    color: Theme.textSecondary
                }
            }

            Button {
                text: "Import Plugin"
                enabled: !root.locked
                opacity: root.locked ? 0.45 : 1.0

                background: Rectangle {
                    radius: 6
                    color: parent.enabled ? Theme.navyPrimary : Theme.bgTertiary
                    implicitWidth: 132
                    implicitHeight: 38
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

        Rectangle {
            Layout.fillWidth: true
            height: root.locked ? 72 : 0
            radius: 8
            color: "#FFF1F2"
            border.color: "#FECACA"
            visible: root.locked

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                AppIcon { name: "warning"; size: 20; color: Theme.statusRed }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: "Plugin changes are locked while recording"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        color: Theme.statusRed
                    }
                    Text {
                        text: "Stop recording to add, remove, or change plugin settings."
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        color: Theme.statusRed
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: root.importError.length > 0 ? 92 : 0
            radius: 8
            color: "#FFF1F2"
            border.color: "#FECACA"
            visible: root.importError.length > 0

            RowLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 14

                AppIcon { name: "close"; size: 20; color: Theme.statusRed }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        text: "Import Plugin Failed"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                        color: Theme.statusRed
                    }
                    Text {
                        text: root.importError
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        color: Theme.textSecondary
                    }
                }
                Button {
                    text: "Choose Another Folder"
                    enabled: !root.locked
                    onClicked: folderDialog.open()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 8
            color: Theme.bgPrimary
            border.color: Theme.borderColor
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    height: 48
                    color: Theme.bgPrimary
                    border.color: Theme.divider

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 24
                        anchors.rightMargin: 24
                        spacing: 16

                        HeaderLabel { text: "Plugin"; Layout.preferredWidth: 220 }
                        HeaderLabel { text: "Type"; Layout.preferredWidth: 88 }
                        HeaderLabel { text: "Version"; Layout.preferredWidth: 84 }
                        HeaderLabel { text: "API"; Layout.preferredWidth: 72 }
                        HeaderLabel { text: "Status"; Layout.preferredWidth: 148 }
                        HeaderLabel { text: "Devices"; Layout.preferredWidth: 82 }
                        HeaderLabel { text: "Enabled"; Layout.preferredWidth: 76 }
                        Item { Layout.fillWidth: true }
                    }
                }

                ListView {
                    id: pluginListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 0

                    model: ListModel { id: pluginListModel }

                    delegate: Rectangle {
                        width: pluginListView.width
                        height: 72
                        color: rowHover.containsMouse ? Theme.bgSecondary : Theme.bgPrimary
                        border.color: Theme.divider

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 24
                            anchors.rightMargin: 24
                            spacing: 16

                            RowLayout {
                                Layout.preferredWidth: 220
                                spacing: 10
                                AppIcon {
                                    name: model.type === "bundled" ? "camera" : "gear"
                                    size: 18
                                    color: Theme.textSecondary
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: model.name
                                        font.family: Theme.fontPrimary
                                        font.pixelSize: 14
                                        font.weight: Font.Medium
                                        color: Theme.textPrimary
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: model.statusMessage || ""
                                        font.family: Theme.fontPrimary
                                        font.pixelSize: 11
                                        color: Theme.textSecondary
                                        elide: Text.ElideRight
                                        visible: text.length > 0
                                        Layout.fillWidth: true
                                    }
                                }
                            }

                            Badge {
                                Layout.preferredWidth: 88
                                text: model.type === "bundled" ? "Bundled" : "Linked"
                                tone: model.type === "bundled" ? "blue" : "green"
                            }

                            Text {
                                text: "v" + model.version
                                font.family: Theme.fontMono
                                font.pixelSize: 12
                                color: Theme.textSecondary
                                Layout.preferredWidth: 84
                            }

                            Badge {
                                Layout.preferredWidth: 72
                                text: "API v" + (model.apiVersion || "?")
                                tone: "gray"
                            }

                            Badge {
                                Layout.preferredWidth: 148
                                text: model.restartRequired ? "Restart required" : model.status
                                tone: model.restartRequired ? "amber" : (model.status === "OK" ? "green" : "red")
                                dot: true
                            }

                            Text {
                                text: model.deviceCount
                                font.family: Theme.fontPrimary
                                font.pixelSize: 13
                                color: Theme.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                                Layout.preferredWidth: 82
                            }

                            Switch {
                                checked: model.enabled
                                enabled: model.canToggle
                                opacity: model.canToggle ? 1.0 : 0.42
                                Layout.preferredWidth: 76

                                ToolTip.visible: !enabled && hovered
                                ToolTip.text: root.locked ? "Not available while recording" : "Bundled plugin cannot be disabled"
                                ToolTip.delay: 200

                                onToggled: appController.togglePlugin(model.path, checked)
                            }

                            Item { Layout.fillWidth: true }

                            Button {
                                text: "Remove"
                                visible: model.type === "linked"
                                enabled: model.canRemove
                                onClicked: appController.removePlugin(model.path)
                            }

                            Text {
                                text: "\u203A"
                                font.pixelSize: 18
                                color: Theme.textTertiary
                                Layout.preferredWidth: 22

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.navigateToDetail(model.path)
                                }
                            }
                        }

                        MouseArea {
                            id: rowHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }
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
            var path = selectedFolder.toString().replace("file://", "")
            if (appController.importPlugin(path)) {
                root.importError = ""
            } else {
                root.importError = "The selected directory is not a valid MiceCam plugin."
            }
        }
    }

    component HeaderLabel : Text {
        font.family: Theme.fontPrimary
        font.pixelSize: 12
        font.weight: Font.Medium
        color: Theme.textSecondary
        verticalAlignment: Text.AlignVCenter
    }

    component Badge : Rectangle {
        property string text: ""
        property string tone: "gray"
        property bool dot: false
        height: 26
        radius: 7
        color: tone === "green" ? "#E8F5E9"
             : tone === "red" ? "#FFEBEE"
             : tone === "amber" ? "#FFF7ED"
             : tone === "blue" ? Theme.navyTint
             : Theme.bgSecondary
        Row {
            anchors.centerIn: parent
            spacing: 6
            Rectangle {
                width: 8; height: 8; radius: 4
                visible: dot
                color: tone === "green" ? Theme.statusGreen
                     : tone === "red" ? Theme.statusRed
                     : tone === "amber" ? Theme.statusAmber
                     : Theme.textTertiary
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: parent.parent.text
                font.family: Theme.fontPrimary
                font.pixelSize: 12
                color: tone === "green" ? "#2E7D32"
                     : tone === "red" ? Theme.statusRed
                     : tone === "amber" ? Theme.statusAmber
                     : tone === "blue" ? Theme.navyPrimary
                     : Theme.textSecondary
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
