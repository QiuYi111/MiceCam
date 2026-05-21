import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    anchors.fill: parent
    color: "#99000000"
    visible: false
    z: 200

    property var items: []

    signal adjustSettings()
    signal dismissed()
    signal forceStart()

    function open() { visible = true }
    function close() { visible = false }

    Keys.onEscapePressed: root.close()

    Rectangle {
        id: modalPanel
        anchors.centerIn: parent
        width: 420
        height: col.implicitHeight + 48
        radius: 14
        color: "white"

        ColumnLayout {
            id: col
            anchors.fill: parent
            anchors.margins: 24
            spacing: 0

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                Layout.bottomMargin: 16
                width: 48; height: 48; radius: 24
                color: "#FEF3C7"

                Text {
                    anchors.centerIn: parent
                    text: "!"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 24
                    font.weight: Font.Bold
                    color: Theme.statusAmber
                }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 8
                text: "Preflight Check Failed"
                font.family: Theme.fontPrimary
                font.pixelSize: 18
                font.weight: Font.Bold
                color: Theme.textPrimary
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 20
                Layout.fillWidth: true
                Layout.preferredWidth: 360
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: "One or more preflight checks failed. Resolve the issues below before starting recording."
                font.family: Theme.fontPrimary
                font.pixelSize: 13
                color: Theme.textSecondary
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.items
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        height: 52
                        radius: 8
                        color: modelData.severity === 2 ? "#FEF2F2" : "#FFFBEB"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 12

                            Rectangle {
                                width: 20; height: 20; radius: 10
                                color: modelData.severity === 2 ? "#FEE2E2" : "#FEF3C7"
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.title ? modelData.title.charAt(0) : "!"
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                    color: modelData.severity === 2 ? Theme.statusRed : Theme.statusAmber
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    text: modelData.title || ""
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 13
                                    font.weight: Font.DemiBold
                                    color: Theme.textPrimary
                                }
                                Text {
                                    text: modelData.message || ""
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 11
                                    color: Theme.textSecondary
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true; Layout.minimumHeight: 16 }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                spacing: 12

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 8
                    color: "transparent"
                    border.color: Theme.bgTertiary
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "Cancel"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { root.dismissed(); root.close() }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 8
                    color: Theme.statusAmber

                    Text {
                        anchors.centerIn: parent
                        text: "Start Anyway"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { root.forceStart(); root.close() }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 40
                    radius: 8
                    color: Theme.navyPrimary

                    Text {
                        anchors.centerIn: parent
                        text: "Adjust Settings"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { root.adjustSettings(); root.close() }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: function(mouse) {
            if (!modalPanel.contains(modalPanel.mapFromItem(root, mouse.x, mouse.y))) {
                root.close()
            }
        }
    }
}
