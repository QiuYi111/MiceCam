import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Flickable {
    id: root
    contentHeight: mainCol.height
    clip: true

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
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.parent.parent.currentViewIndex = 0 }
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
            implicitHeight: listCol.implicitHeight + 24
            color: "white"
            radius: 10
            border.color: Theme.borderColor
            border.width: 1

            ColumnLayout {
                id: listCol
                anchors.fill: parent
                anchors.topMargin: 12
                anchors.bottomMargin: 12
                spacing: 0

                LogRow {
                    title: "Log level"
                    description: "Minimum severity level to record."
                    Layout.fillWidth: true
                    controlItem: RowLayout {
                        spacing: 0
                        Repeater {
                            model: ["Trace", "Debug", "Info", "Warn", "Error"]
                            delegate: Rectangle {
                                required property string modelData
                                required property int index
                                width: 64
                                height: 28
                                radius: index === 0 ? 6 : (index === 4 ? 6 : 0)
                                color: modelData === "Info" ? Theme.navyPrimary : Theme.bgSecondary
                                border.color: Theme.borderColor
                                border.width: modelData === "Info" ? 0 : 1

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 12
                                    font.weight: modelData === "Info" ? Font.Bold : Font.Normal
                                    color: modelData === "Info" ? "white" : Theme.textSecondary
                                }

                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                            }
                        }
                    }
                }

                LogDivider {}

                LogRow {
                    title: "Verbose session diagnostics"
                    description: "Include additional camera, encoding, and system diagnostics in logs."
                    Layout.fillWidth: true
                    controlItem: LogHigSwitch { checked: false }
                }

                LogDivider {}

                LogRow {
                    title: "Output directory"
                    description: "Where log files are stored on disk."
                    Layout.fillWidth: true
                    controlItem: RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            radius: 6
                            color: Theme.bgSecondary
                            border.color: Theme.borderColor
                            border.width: 1

                            Text {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                verticalAlignment: Text.AlignVCenter
                                text: "~/Library/Logs/MiceCam/"
                                font.family: Theme.fontMono
                                font.pixelSize: 12
                                color: Theme.textSecondary
                            }
                        }
                        Button {
                            implicitWidth: 32
                            implicitHeight: 28
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

                LogDivider {}

                LogRow {
                    title: "Log file actions"
                    description: "Manage log files on disk."
                    Layout.fillWidth: true
                    controlItem: RowLayout {
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

                LogDivider {}

                LogRow {
                    title: "Recent log preview"
                    description: "Latest log output from the current session."
                    Layout.fillWidth: true
                    controlItem: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        RowLayout {
                            Layout.fillWidth: true
                            Rectangle { width: 6; height: 6; radius: 3; color: Theme.statusGreen; Layout.alignment: Qt.AlignVCenter }
                            Text { text: "Live"; font.family: Theme.fontPrimary; font.pixelSize: 11; font.weight: Font.Bold; color: Theme.statusGreen }
                            Item { Layout.fillWidth: true }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            height: 140
                            radius: 6
                            color: "#FAFAFA"
                            border.color: Theme.borderColor
                            border.width: 1
                            clip: true

                            Column {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 3
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.textSecondary; text: "[INFO]  Session started \u2014 MiceCam v2.0.0" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.navyPrimary; text: "[INFO]  Camera OAK-D-1 connected (192.168.1.10)" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.navyPrimary; text: "[INFO]  Recording started \u2014 cam_01_2026-05-14.mp4" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.statusAmber; text: "[WARN]  Frame drop detected \u2014 cam_02 (1.2%)" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.navyPrimary; text: "[INFO]  Encoding H.265 @ 30fps \u2014 quality 85" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.navyPrimary; text: "[INFO]  Storage: 245 GB free / 500 GB total" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.textTertiary; text: "[DEBUG] Buffer pool: 48/64 frames allocated" }
                                Text { font.family: Theme.fontMono; font.pixelSize: 11; color: Theme.navyPrimary; text: "[INFO]  Watchdog healthy \u2014 all cameras active" }
                            }
                        }
                    }
                }
            }
        }
    }

    component LogRow : RowLayout {
        id: logRow
        property string title: ""
        property string description: ""
        property alias controlItem: controlSlot.data
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        spacing: 24

        ColumnLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 300
            spacing: 2
            Text { text: logRow.title; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary; Layout.fillWidth: true }
            Text { text: logRow.description; font.family: Theme.fontPrimary; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        }

        Item {
            id: controlSlot
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
        }
    }

    component LogDivider : Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        height: 1
        color: Theme.divider
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
