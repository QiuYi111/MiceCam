import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    height: 56
    color: Theme.bgPrimary

    property bool isRecording: false
    property bool canStartRecording: false
    property string recordText: "Record"
    readonly property bool recordActionEnabled: isRecording || canStartRecording
    readonly property bool startActionVisible: !isRecording && canStartRecording
    property var alertModel: null
    property string elapsedText: "00:00"

    signal fullscreenClicked()
    signal preflightTriggered()
    signal settingsClicked()
    signal recordClicked()

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
                readonly property color actionColor: root.isRecording
                    ? Theme.recordRed
                    : (root.canStartRecording ? Theme.statusGreen : Theme.bgTertiary)
                width: root.recordActionEnabled ? 104 : 128
                height: 36
                radius: 8
                color: actionColor
                opacity: root.recordActionEnabled ? 1.0 : 0.72

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    AppIcon {
                        name: root.isRecording ? "stop" : (root.startActionVisible ? "play" : "warning")
                        size: 16
                        color: root.recordActionEnabled ? "white" : Theme.textSecondary
                    }

                    Text {
                        text: root.recordText
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: root.recordActionEnabled ? "white" : Theme.textSecondary
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: root.recordActionEnabled
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: root.recordClicked()
                }
            }

            Rectangle {
                width: 88
                height: 36
                radius: 8
                color: Theme.bgSecondary
                visible: root.isRecording

                Text {
                    anchors.centerIn: parent
                    text: root.elapsedText
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
                    id: alertBadge
                    anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: -4; anchors.rightMargin: -4
                    width: 18; height: 18; radius: 9; color: Theme.statusRed
                    visible: alertBadgeText.text !== "0"
                    Text {
                        id: alertBadgeText
                        anchors.centerIn: parent
                        text: root.alertModel ? root.alertModel.badgeCount : "0"
                        color: "white"; font.pixelSize: 11; font.weight: Font.Bold
                    }
                }
                MouseArea { anchors.fill: parent; onClicked: notifyPopup.open() }
                NotificationPopup {
                    id: notifyPopup
                    alertModel: root.alertModel
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
