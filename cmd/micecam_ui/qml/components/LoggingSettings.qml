import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Flickable {
    id: root
    contentHeight: mainCol.height
    clip: true

    signal navigateBack()

    ScrollBar.vertical: ScrollBar {}

    ColumnLayout {
        id: mainCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 32
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                text: "\u2039 Cameras"
                font.family: Theme.fontPrimary
                font.pixelSize: 14
                color: Theme.navyPrimary
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.navigateBack() }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text { text: "Logging"; font.family: Theme.fontPrimary; font.pixelSize: 28; font.weight: Font.Bold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            Row {
                spacing: 6
                AppIcon {
                    name: "check"
                    size: 12
                    color: Theme.statusGreen
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text { text: "All changes saved automatically"; color: Theme.statusGreen; font.family: Theme.fontPrimary; font.pixelSize: 12; font.weight: Font.Medium }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: settingsContent.implicitHeight + 24
            color: "white"
            radius: 10
            border.color: Theme.borderColor
            border.width: 1

            ColumnLayout {
                id: settingsContent
                anchors.fill: parent
                anchors.topMargin: 12
                anchors.bottomMargin: 12
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    height: rowContent1.height + 16
                    color: "transparent"

                    RowLayout {
                        id: rowContent1
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 16

                        ColumnLayout {
                            Layout.preferredWidth: 260
                            spacing: 2
                            Text { text: "Log level"; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary }
                            Text { text: "Minimum severity level to record."; font.family: Theme.fontPrimary; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }

                        Item { Layout.fillWidth: true }

                        Row {
                            spacing: 0
                            Layout.alignment: Qt.AlignVCenter
                            Repeater {
                                model: ["Trace", "Debug", "Info", "Warn", "Error"]
                                delegate: Rectangle {
                                    required property string modelData
                                    required property int index
                                    width: 72
                                    height: 30
                                    color: modelData === appController.settings.logLevel ? Theme.navyPrimary : Theme.bgSecondary
                                    border.color: Theme.borderColor
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.family: Theme.fontPrimary
                                        font.pixelSize: 12
                                        font.weight: modelData === appController.settings.logLevel ? Font.Bold : Font.Normal
                                        color: modelData === appController.settings.logLevel ? "white" : Theme.textSecondary
                                    }

                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    height: 1
                    color: Theme.divider
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    height: rowContent2.height + 16
                    color: "transparent"

                    RowLayout {
                        id: rowContent2
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 16

                        ColumnLayout {
                            Layout.preferredWidth: 260
                            spacing: 2
                            Text { text: "Verbose session diagnostics"; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary }
                            Text { text: "Include additional camera, encoding, and system diagnostics in logs."; font.family: Theme.fontPrimary; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }

                        Item { Layout.fillWidth: true }

                        LogHigSwitch {
                            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
                            checked: false
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    height: 1
                    color: Theme.divider
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    height: rowContent3.height + 16
                    color: "transparent"

                    RowLayout {
                        id: rowContent3
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 16

                        ColumnLayout {
                            Layout.preferredWidth: 260
                            spacing: 2
                            Text { text: "Output directory"; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary }
                            Text { text: "Where log files are stored on disk."; font.family: Theme.fontPrimary; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true
                                height: 30
                                radius: 6
                                color: Theme.bgSecondary
                                border.color: Theme.borderColor
                                border.width: 1

                                Text {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    verticalAlignment: Text.AlignVCenter
                                    text: appController.settings.outputDirectory
                                    font.family: Theme.fontMono
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                }
                            }

                            Button {
                                implicitWidth: 36
                                implicitHeight: 30
                                contentItem: AppIcon {
                                    name: "disk"
                                    size: 14
                                    color: Theme.textSecondary
                                    anchors.centerIn: parent
                                }
                                background: Rectangle {
                                    radius: 6
                                    border.color: Theme.borderColor
                                    border.width: 1
                                    color: parent.down ? Theme.bgSecondary : "white"
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    height: 1
                    color: Theme.divider
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    height: rowContent4.height + 16
                    color: "transparent"

                    RowLayout {
                        id: rowContent4
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        spacing: 16

                        ColumnLayout {
                            Layout.preferredWidth: 260
                            spacing: 2
                            Text { text: "Log file actions"; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary }
                            Text { text: "Manage log files on disk."; font.family: Theme.fontPrimary; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }

                        Item { Layout.fillWidth: true }

                        RowLayout {
                            spacing: 8
                            Button {
                                contentItem: Row {
                                    spacing: 4
                                    AppIcon {
                                        name: "disk"
                                        size: 12
                                        color: Theme.navyPrimary
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "Open Log Folder"
                                        font.family: Theme.fontPrimary
                                        font.pixelSize: 12
                                        color: Theme.navyPrimary
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                background: Rectangle {
                                    implicitHeight: 28
                                    radius: 6
                                    border.color: Theme.borderColor
                                    border.width: 1
                                    color: parent.down ? Theme.bgSecondary : "white"
                                }
                            }
                            Button {
                                contentItem: Row {
                                    spacing: 4
                                    AppIcon {
                                        name: "logging"
                                        size: 12
                                        color: Theme.navyPrimary
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                    Text {
                                        text: "Reveal Session Files"
                                        font.family: Theme.fontPrimary
                                        font.pixelSize: 12
                                        color: Theme.navyPrimary
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                background: Rectangle {
                                    implicitHeight: 28
                                    radius: 6
                                    border.color: Theme.borderColor
                                    border.width: 1
                                    color: parent.down ? Theme.bgSecondary : "white"
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    height: 1
                    color: Theme.divider
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 16
                    Layout.rightMargin: 16
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Recent log preview"; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary }
                        Item { Layout.fillWidth: true }
                        Row {
                            spacing: 4
                            Rectangle { width: 6; height: 6; radius: 3; color: Theme.statusGreen; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "Live"; font.family: Theme.fontPrimary; font.pixelSize: 11; font.weight: Font.Bold; color: Theme.statusGreen }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 140
                        height: 160
                        radius: 8
                        color: "#FAFAFA"
                        border.color: Theme.borderColor
                        border.width: 1
                        clip: true

                        Column {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 4
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.textSecondary; text: "[INFO]  Session started \u2014 MiceCam v2.0.0" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.navyPrimary; text: "[INFO]  Camera OAK-D-1 connected (192.168.1.10)" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.navyPrimary; text: "[INFO]  Recording started \u2014 cam_01_2026-05-14.mp4" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.statusAmber; text: "[WARN]  Frame drop detected \u2014 cam_02 (1.2%)" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.navyPrimary; text: "[INFO]  Encoding H.265 @ 30fps \u2014 quality 85" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.navyPrimary; text: "[INFO]  Storage: 245 GB free / 500 GB total" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.textTertiary; text: "[DEBUG] Buffer pool: 48/64 frames allocated" }
                            Text { font.family: Theme.fontMono; font.pixelSize: 12; color: Theme.navyPrimary; text: "[INFO]  Watchdog healthy \u2014 all cameras active" }
                        }
                    }
                }
            }
        }
    }

    component LogHigSwitch : Rectangle {
        id: logSwitchRoot
        width: 52
        height: 30
        radius: 15
        property bool checked: false
        color: checked ? Theme.navyPrimary : Theme.bgTertiary
        Behavior on color { ColorAnimation { duration: 150 } }

        Rectangle {
            id: logKnob
            width: 26
            height: 26
            radius: 13
            color: "white"
            border.color: "#C0C0C0"
            border.width: 0.5
            x: logSwitchRoot.checked ? logSwitchRoot.width - width - 2 : 2
            y: 2
            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: logSwitchRoot.checked = !logSwitchRoot.checked
        }
    }
}
