import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    width: 240
    color: Theme.bgSecondary

    signal viewChanged(int index)
    signal cameraSelected(var cameraData)

    property int activeViewIndex: 0
    property string _selectedCameraName: ""

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.divider
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 0

        Rectangle {
            id: homeEntry
            Layout.fillWidth: true
            height: 40
            radius: 8
            color: root.activeViewIndex === 0 ? Theme.navyTint : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 12
                spacing: 12

                AppIcon {
                    name: "camera"
                    size: 16
                    color: root.activeViewIndex === 0 ? Theme.navyPrimary : Theme.textSecondary
                }

                Text {
                    text: "Cameras"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 14
                    font.weight: root.activeViewIndex === 0 ? Font.Medium : Font.Normal
                    color: root.activeViewIndex === 0 ? Theme.navyPrimary : Theme.textPrimary
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: root.viewChanged(0)
            }
        }

        Item { height: 4 }

        Text {
            text: "CAMERA SOURCES"
            font.family: Theme.fontPrimary
            font.pixelSize: 11
            font.weight: Font.Bold
            color: Theme.textTertiary
            Layout.bottomMargin: 8
            Layout.leftMargin: 16
        }

        ListView {
            id: sourceList
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            model: appController.sourceModel
            interactive: false

            delegate: Rectangle {
                id: sourceItem
                width: ListView.view.width
                height: 44 + (model.devices ? model.devices.length * 36 : 0) + (model.deviceCount === 0 ? 28 : 0)
                radius: 8
                color: "transparent"
                property var sourceDevices: model.devices || []

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 40
                        radius: 8
                        color: sourceMouse.containsMouse ? Theme.bgPrimary : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                text: model.isExpanded ? "\u2304" : "\u203A"
                                font.pixelSize: 16
                                color: Theme.textSecondary
                                Layout.preferredWidth: 12
                            }

                            Text {
                                text: model.sourceName
                                font.family: Theme.fontPrimary
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: model.restartRequired ? "restart" : ""
                                font.family: Theme.fontPrimary
                                font.pixelSize: 11
                                color: Theme.statusAmber
                                visible: model.restartRequired
                            }

                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: model.diagnostics === 0 ? Theme.statusGreen
                                     : model.diagnostics === 2 ? Theme.textTertiary
                                     : model.diagnostics === 3 ? Theme.statusRed
                                     : Theme.statusAmber
                            }
                        }

                        MouseArea {
                            id: sourceMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                        }
                    }

                    Repeater {
                        model: sourceItem.sourceDevices
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            height: 36
                            radius: 8
                            color: root.activeViewIndex === 6 && root._selectedCameraName === modelData.name ? Theme.navyTint : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 36
                                anchors.rightMargin: 12
                                spacing: 10

                                AppIcon {
                                    name: "camera"
                                    size: 14
                                    color: Theme.textSecondary
                                }

                                Text {
                                    text: modelData.name || modelData.displayName
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 13
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Rectangle {
                                    width: 8; height: 8; radius: 4
                                    color: modelData.statusCode === 0 ? Theme.statusGreen : Theme.statusRed
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root._selectedCameraName = modelData.name || modelData.displayName
                                    root.cameraSelected(modelData)
                                }
                            }
                        }
                    }

                    Text {
                        Layout.leftMargin: 36
                        Layout.rightMargin: 12
                        Layout.fillWidth: true
                        height: visible ? 28 : 0
                        text: model.deviceCount === 0 ? (model.diagnosticsMessage || model.statusLabel) : ""
                        font.family: Theme.fontPrimary
                        font.pixelSize: 12
                        color: Theme.textSecondary
                        elide: Text.ElideRight
                        visible: model.deviceCount === 0
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Repeater {
                model: [
                    { name: "Encoding", icon: "encoding" },
                    { name: "Alerts", icon: "alerts" },
                    { name: "Logging", icon: "logging" },
                    { name: "Plugins", icon: "gear" },
                    { name: "About", icon: "about" }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    radius: 8
                    color: root.activeViewIndex === (index + 1) ? Theme.navyTint : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12

                        AppIcon {
                            name: modelData.icon
                            size: 16
                            color: root.activeViewIndex === (index + 1) ? Theme.navyPrimary : Theme.textSecondary
                        }

                        Text {
                            text: modelData.name
                            font.family: Theme.fontPrimary
                            font.pixelSize: 14
                            font.weight: root.activeViewIndex === (index + 1) ? Font.Medium : Font.Normal
                            color: root.activeViewIndex === (index + 1) ? Theme.navyPrimary : Theme.textPrimary
                            Layout.fillWidth: true
                        }

                        AppIcon {
                            name: "chevron-right"
                            size: 10
                            color: Theme.textTertiary
                            visible: root.activeViewIndex !== (index + 1)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.viewChanged(index + 1)
                    }
                }
            }
        }
    }
}
