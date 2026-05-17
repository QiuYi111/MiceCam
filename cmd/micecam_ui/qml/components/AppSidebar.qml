import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    width: 240
    color: Theme.bgSecondary

    signal viewChanged(int index)
    signal cameraSelected(string name, int status)

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
            text: "DEVICES"
            font.family: Theme.fontPrimary
            font.pixelSize: 11
            font.weight: Font.Bold
            color: Theme.textTertiary
            Layout.bottomMargin: 8
            Layout.leftMargin: 16
        }

        ListView {
            id: cameraList
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            model: appController.cameraModel
            interactive: false

            delegate: Rectangle {
                id: cameraItem
                width: ListView.view.width
                height: 40
                radius: 8
                color: {
                    if (root.activeViewIndex === 6 && root._selectedCameraName === model.name) return Theme.navyTint
                    return "transparent"
                }

                property string camName: model.name
                property int camStatus: model.status

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12
                    spacing: 12

                    Item {
                        width: 18; height: 18
                        Rectangle {
                            width: 12; height: 10; radius: 2; color: Theme.textSecondary
                            anchors.centerIn: parent
                            anchors.horizontalCenterOffset: -2
                        }
                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.fillStyle = Qt.binding(function() { return Theme.textSecondary; });
                                ctx.beginPath();
                                ctx.moveTo(12, 6); ctx.lineTo(16, 4); ctx.lineTo(16, 14); ctx.lineTo(12, 12);
                                ctx.closePath(); ctx.fill();
                            }
                        }
                    }

                    Text {
                        text: model.name
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: model.status === 0 ? Theme.statusGreen : (model.status === 1 ? Theme.statusAmber : Theme.statusRed)
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root._selectedCameraName = cameraItem.camName
                        root.cameraSelected(cameraItem.camName, cameraItem.camStatus)
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
                    { name: "Plugins", icon: "logging" },
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
