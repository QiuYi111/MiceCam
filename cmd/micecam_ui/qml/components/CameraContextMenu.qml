import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Menu {
    id: root

    property string cameraName: ""
    signal configureClicked()
    signal fullscreenClicked()
    signal removeClicked()

    padding: 4
    topPadding: 4
    bottomPadding: 4

    background: Rectangle {
        color: "white"
        radius: 10
        border.color: Theme.bgTertiary
        border.width: 1

        Rectangle {
            anchors.fill: parent
            anchors.margins: -4
            radius: 14
            color: "transparent"
            border.width: 0

            layer.enabled: true
            layer.effect: Item {
            }
        }

        layer.enabled: true
    }

    delegate: MenuItem {
        id: menuItem
        width: 180
        height: 36
        padding: 0
        leftPadding: 12
        rightPadding: 12

        contentItem: RowLayout {
            spacing: 10

            Rectangle {
                width: 18
                height: 18
                radius: 4
                color: "transparent"

                Text {
                    anchors.centerIn: parent
                    font.family: Theme.fontPrimary
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    color: menuItem.enabled ? Theme.textPrimary : Theme.textTertiary
                    text: {
                        if (menuItem.text === "Configure") return "C"
                        if (menuItem.text === "Fullscreen") return "F"
                        if (menuItem.text === "Remove") return "R"
                        return ""
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: menuItem.text
                font.family: Theme.fontPrimary
                font.pixelSize: 13
                font.weight: Font.Medium
                color: {
                    if (!menuItem.enabled) return Theme.textTertiary
                    if (menuItem.text === "Remove") return Theme.statusRed
                    return Theme.textPrimary
                }
            }
        }

        background: Rectangle {
            radius: 6
            color: menuItem.hovered ? Theme.bgSecondary : "transparent"
        }

        onTriggered: {
            if (text === "Configure") root.configureClicked()
            else if (text === "Fullscreen") root.fullscreenClicked()
            else if (text === "Remove") root.removeClicked()
        }
    }

    Action { text: "Configure" }
    Action { text: "Fullscreen" }
    Action { text: "Remove"; enabled: false }
}
