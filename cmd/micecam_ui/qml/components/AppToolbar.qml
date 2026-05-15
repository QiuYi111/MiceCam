import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    height: 56
    color: Theme.bgPrimary

    signal fullscreenClicked()
    signal preflightTriggered()
    signal settingsClicked()

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.bgTertiary
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        Row {
            spacing: 4

            Rectangle {
                id: recordBtn
                width: 110
                height: 36
                radius: 8
                color: Theme.recordRed

                property bool isRecording: true

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    Rectangle {
                        width: 18; height: 18; radius: 9; color: "white"
                        Rectangle {
                            width: 8; height: 8; radius: 1; color: Theme.recordRed
                            anchors.centerIn: parent
                        }
                    }

                    Text {
                        text: recordBtn.isRecording ? "Stop" : "Record"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "white"
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!recordBtn.isRecording) {
                            root.preflightTriggered()
                        }
                    }
                }
            }

            Rectangle {
                width: 88
                height: 36
                radius: 8
                color: Theme.bgSecondary
                visible: recordBtn.isRecording

                Text {
                    anchors.centerIn: parent
                    text: "00:42:17"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: Theme.recordRed
                }

                MouseArea {
                    anchors.fill: parent
                }
            }
        }

        Item { Layout.fillWidth: true }

        RowLayout {
            spacing: 12

            Rectangle {
                width: 36; height: 36; radius: 8; color: "transparent"; border.color: Theme.bgTertiary; border.width: 1
                AppIcon { anchors.centerIn: parent; name: "alerts"; size: 18 }
                Rectangle {
                    anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: -4; anchors.rightMargin: -4
                    width: 18; height: 18; radius: 9; color: Theme.statusRed
                    Text { anchors.centerIn: parent; text: "3"; color: "white"; font.pixelSize: 11; font.weight: Font.Bold }
                }
                MouseArea { anchors.fill: parent; onClicked: notifyPopup.open() }
                NotificationPopup {
                    id: notifyPopup
                    y: parent.height + 8
                    x: Math.max(0, root.width - notifyPopup.width - 12)
                }
            }

            Rectangle {
                width: 36; height: 36; radius: 8; color: "transparent"; border.color: Theme.bgTertiary; border.width: 1
                AppIcon { anchors.centerIn: parent; name: "fullscreen"; size: 18 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.fullscreenClicked()
                }
            }

            Rectangle {
                width: 110; height: 36; radius: 8; color: "transparent"; border.color: Theme.bgTertiary; border.width: 1
                RowLayout {
                    anchors.centerIn: parent; spacing: 8
                    AppIcon { name: "gear"; size: 16 }
                    Text { text: "Settings"; font.family: Theme.fontPrimary; font.pixelSize: 14; color: Theme.textPrimary }
                    AppIcon { name: "chevron-right"; size: 8; color: Theme.textSecondary; rotation: 90 }
                }
                MouseArea { anchors.fill: parent; onClicked: root.settingsClicked() }
            }
        }
    }
}
