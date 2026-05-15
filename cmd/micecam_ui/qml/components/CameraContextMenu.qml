import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string cameraName: ""
    property bool menuVisible: false
    property real menuX: 0
    property real menuY: 0
    signal configureClicked()
    signal fullscreenClicked()
    signal removeClicked()

    visible: menuVisible
    z: 999
    width: 200
    height: menuCol.height + 16
    color: "white"
    radius: 10
    border.color: Theme.bgTertiary
    border.width: 1

    x: menuX
    y: menuY

    onMenuVisibleChanged: {
        if (menuVisible) {
            closeTimer.stop()
        }
    }

    Column {
        id: menuCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        spacing: 2

        Rectangle {
            width: parent.width
            height: 36
            radius: 6
            color: configureHover.containsMouse ? Theme.bgSecondary : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Rectangle {
                    width: 18; height: 18; radius: 4; color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        font.family: Theme.fontPrimary; font.pixelSize: 12; font.weight: Font.Medium
                        color: Theme.textPrimary; text: "C"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Configure"
                    font.family: Theme.fontPrimary; font.pixelSize: 13; font.weight: Font.Medium
                    color: Theme.textPrimary
                }
            }

            MouseArea {
                id: configureHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.menuVisible = false
                    root.configureClicked()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 36
            radius: 6
            color: fullscreenHover.containsMouse ? Theme.bgSecondary : "transparent"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Rectangle {
                    width: 18; height: 18; radius: 4; color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        font.family: Theme.fontPrimary; font.pixelSize: 12; font.weight: Font.Medium
                        color: Theme.textPrimary; text: "F"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Fullscreen"
                    font.family: Theme.fontPrimary; font.pixelSize: 13; font.weight: Font.Medium
                    color: Theme.textPrimary
                }
            }

            MouseArea {
                id: fullscreenHover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.menuVisible = false
                    root.fullscreenClicked()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 36
            radius: 6
            color: "transparent"
            opacity: 0.4

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 10

                Rectangle {
                    width: 18; height: 18; radius: 4; color: "transparent"
                    Text {
                        anchors.centerIn: parent
                        font.family: Theme.fontPrimary; font.pixelSize: 12; font.weight: Font.Medium
                        color: Theme.textTertiary; text: "R"
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: "Remove"
                    font.family: Theme.fontPrimary; font.pixelSize: 13; font.weight: Font.Medium
                    color: Theme.textTertiary
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.menuVisible = false
            }
        }
    }

    Timer {
        id: closeTimer
        interval: 8000
        onTriggered: root.menuVisible = false
    }

    function show(px, py) {
        menuX = px
        menuY = py
        menuVisible = true
        closeTimer.start()
    }

    function hide() {
        menuVisible = false
        closeTimer.stop()
    }
}
