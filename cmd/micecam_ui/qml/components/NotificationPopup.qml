import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Popup {
    id: root
    width: 340
    height: 440
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: "white"
        radius: 12
        border.color: Theme.bgTertiary
        border.width: 1
        layer.enabled: true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            spacing: 8

            Text {
                text: "Notifications"
                font.family: Theme.fontPrimary
                font.pixelSize: 15
                font.weight: Font.Bold
                color: Theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: true
                width: clearAllText.implicitWidth + 16
                height: 24
                radius: 4
                color: "transparent"

                Text {
                    id: clearAllText
                    anchors.centerIn: parent
                    text: "Clear All"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: Theme.navyPrimary
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgTertiary }

        ListView {
            id: alertList
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 240
            Layout.maximumHeight: 320
            clip: true
            spacing: 0

            model: ListModel {
                ListElement { severity: 2; title: "High drop rate detected on CAM_D"; source: "CAM_D"; relTime: "2 min ago"; badge: 1 }
                ListElement { severity: 1; title: "Encoder fallback on USB-1"; source: "USB-1"; relTime: "5 min ago"; badge: 2 }
                ListElement { severity: 1; title: "High drop rate detected on CAM_C"; source: "CAM_C"; relTime: "6 min ago"; badge: 0 }
                ListElement { severity: 0; title: "Camera disconnect recovered on CAM_B"; source: "CAM_B"; relTime: "12 min ago"; badge: 0 }
            }

            delegate: Item {
                width: alertList.width
                height: 68

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.topMargin: 2
                    anchors.bottomMargin: 2
                    radius: 8
                    color: mouseArea.containsMouse ? Theme.bgSecondary : "transparent"

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.topMargin: 2
                            width: 8; height: 8; radius: 4
                            color: severity === 2 ? Theme.statusRed : (severity === 1 ? Theme.statusAmber : Theme.statusGreen)
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                Layout.fillWidth: true
                                text: model.title
                                font.family: Theme.fontPrimary
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                wrapMode: Text.NoWrap
                            }

                            Text {
                                text: model.source
                                font.family: Theme.fontPrimary
                                font.pixelSize: 10
                                color: Theme.textTertiary
                            }
                        }

                        ColumnLayout {
                            Layout.alignment: Qt.AlignTop
                            spacing: 4

                            Text {
                                Layout.alignment: Qt.AlignRight
                                text: model.relTime
                                font.family: Theme.fontPrimary
                                font.pixelSize: 10
                                color: Theme.textTertiary
                            }

                            Rectangle {
                                Layout.alignment: Qt.AlignRight
                                visible: model.badge > 0
                                width: badgeText.implicitWidth + 8
                                height: 16
                                radius: 8
                                color: Theme.statusRed

                                Text {
                                    id: badgeText
                                    anchors.centerIn: parent
                                    text: model.badge
                                    color: "white"
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 32
                    height: 1
                    color: Theme.bgTertiary
                    visible: index < alertList.count - 1
                }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgTertiary }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "transparent"

            Text {
                anchors.centerIn: parent
                text: "Show All Alerts"
                font.family: Theme.fontPrimary
                font.pixelSize: 13
                font.weight: Font.Medium
                color: Theme.navyPrimary
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var win = root.parent
                    while (win && !win.currentViewIndex && win.currentViewIndex !== 0) win = win.parent
                    if (win) win.currentViewIndex = 2
                    root.close()
                }
            }
        }
    }
}
