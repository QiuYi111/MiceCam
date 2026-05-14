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

        Rectangle {
            id: recordBtn
            width: isRecording ? 180 : 110
            height: 36
            radius: 8
            color: recordBtn.isRecording ? "white" : Theme.recordRed
            border.color: Theme.recordRed
            border.width: 1
            clip: true

            property bool isRecording: true

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.preferredWidth: recordBtn.isRecording ? 90 : recordBtn.width - 2
                    Layout.fillHeight: true
                    Layout.topMargin: 1
                    Layout.bottomMargin: 1
                    Layout.leftMargin: 1
                    color: Theme.recordRed
                    radius: 7
                    clip: true

                    Rectangle {
                        anchors.right: parent.right
                        width: 10
                        height: parent.height
                        color: Theme.recordRed
                        visible: recordBtn.isRecording
                    }

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
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.topMargin: 1
                    Layout.bottomMargin: 1
                    Layout.rightMargin: 1
                    color: "white"
                    visible: recordBtn.isRecording

                    Text {
                        anchors.centerIn: parent
                        text: "00:42:17"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        color: Theme.recordRed
                    }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!recordBtn.isRecording) {
                        root.preflightTriggered()
                    }
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
