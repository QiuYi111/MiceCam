import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string cameraName: "CAM_A"
    property double cameraFps: 29.97
    property int cameraDrops: 0
    property int cameraStatus: 0
    property bool cameraRecording: true

    signal backClicked()
    signal fullscreenClicked(string name, real fps, int drops, bool isRecording, int status)

    Flickable {
        anchors.fill: parent
        contentHeight: detailContent.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: detailContent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 32
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: "\u2039 Cameras"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 14
                    color: Theme.navyPrimary
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.backClicked()
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 120
                    height: 34
                    radius: 8
                    color: Theme.navyPrimary

                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 6

                        AppIcon {
                            name: "fullscreen"
                            size: 14
                            color: "white"
                        }

                        Text {
                            text: "Fullscreen"
                            font.family: Theme.fontPrimary
                            font.pixelSize: 13
                            font.weight: Font.Medium
                            color: "white"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.fullscreenClicked(root.cameraName, root.cameraFps, root.cameraDrops, root.cameraRecording, root.cameraStatus)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 16

                RowLayout {
                    spacing: 12

                    Text {
                        text: root.cameraName
                        font.family: Theme.fontPrimary
                        font.pixelSize: 28
                        font.weight: Font.Bold
                        color: Theme.textPrimary
                    }

                    Rectangle {
                        width: 10
                        height: 10
                        radius: 5
                        color: root.cameraStatus === 0 ? Theme.statusGreen : (root.cameraStatus === 1 ? Theme.statusAmber : Theme.statusRed)
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: root.cameraStatus === 0 ? "Connected" : (root.cameraStatus === 1 ? "Warning" : "Disconnected")
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        color: root.cameraStatus === 0 ? Theme.statusGreen : (root.cameraStatus === 1 ? Theme.statusAmber : Theme.statusRed)
                        font.weight: Font.Medium
                    }

                    Rectangle {
                        visible: root.cameraRecording
                        width: 8
                        height: 8
                        radius: 4
                        color: Theme.recordRed
                        Layout.alignment: Qt.AlignVCenter

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                            NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                        }
                    }

                    Text {
                        visible: root.cameraRecording
                        text: "REC"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        color: Theme.recordRed
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(root.height * 0.45, 360)
                radius: 12
                color: "#1A1A1E"
                clip: true

                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        var cellW = width / 40
                        var cellH = height / 30
                        for (var gx = 0; gx < 40; gx++) {
                            for (var gy = 0; gy < 30; gy++) {
                                var noise = Math.random() * 12
                                var base = 28 + noise
                                ctx.fillStyle = "rgb(" + Math.floor(base) + "," + Math.floor(base + 2) + "," + Math.floor(base + 6) + ")"
                                ctx.fillRect(gx * cellW, gy * cellH, cellW + 0.5, cellH + 0.5)
                            }
                        }
                        ctx.strokeStyle = "rgba(60, 70, 90, 0.2)"
                        ctx.lineWidth = 0.5
                        for (var lx = 0; lx <= 6; lx++) {
                            var px = lx * width / 6
                            ctx.beginPath(); ctx.moveTo(px, 0); ctx.lineTo(px, height); ctx.stroke()
                        }
                        for (var ly = 0; ly <= 4; ly++) {
                            var py = ly * height / 4
                            ctx.beginPath(); ctx.moveTo(0, py); ctx.lineTo(width, py); ctx.stroke()
                        }
                    }
                    Component.onCompleted: requestPaint()
                }

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 36
                    color: "#B2000000"

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12

                        Text {
                            text: root.cameraFps.toFixed(2) + " fps"
                            color: "white"
                            font.family: Theme.fontPrimary
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: "00:42:17"
                            color: "#99FFFFFF"
                            font.family: Theme.fontMono
                            font.pixelSize: 12
                        }

                        Item { width: 16 }

                        Text {
                            text: root.cameraDrops + " drops"
                            color: root.cameraStatus === 1 ? Theme.statusAmber : "#CCCCCC"
                            font.family: Theme.fontPrimary
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: metricsGrid.height + 32
                color: "white"
                radius: 10
                border.color: Theme.borderColor
                border.width: 1

                GridLayout {
                    id: metricsGrid
                    anchors.fill: parent
                    anchors.margins: 16
                    columns: 4
                    rowSpacing: 12
                    columnSpacing: 16

                    Repeater {
                        model: [
                            { label: "Frame Rate", value: root.cameraFps.toFixed(2) + " fps" },
                            { label: "Frame Drops", value: root.cameraDrops.toString() },
                            { label: "Resolution", value: "1920\u00d71080" },
                            { label: "Encoder", value: "H.265 (HEVC)" },
                            { label: "Bitrate", value: "12.0 Mbps" },
                            { label: "Uptime", value: "00:42:17" },
                            { label: "Buffer", value: "48/64" },
                            { label: "Quality", value: "85%" }
                        ]

                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: modelData.label
                                font.family: Theme.fontPrimary
                                font.pixelSize: 11
                                color: Theme.textTertiary
                                font.weight: Font.Medium
                            }

                            Text {
                                text: modelData.value
                                font.family: Theme.fontPrimary
                                font.pixelSize: 15
                                color: Theme.textPrimary
                                font.weight: Font.DemiBold
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: configContent.height + 24
                color: "white"
                radius: 10
                border.color: Theme.borderColor
                border.width: 1

                ColumnLayout {
                    id: configContent
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
                        height: enabledRow.height + 16
                        color: "transparent"

                        RowLayout {
                            id: enabledRow
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: 16

                            ColumnLayout {
                                Layout.preferredWidth: 260
                                spacing: 2

                                Text {
                                    text: "Camera enabled"
                                    font.family: Theme.fontPrimary
                                    font.weight: Font.Bold
                                    font.pixelSize: 13
                                    color: Theme.textPrimary
                                }

                                Text {
                                    text: "Enable or disable this camera source."
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Rectangle {
                                id: enabledSwitch
                                property bool checked: true
                                width: 52
                                height: 30
                                radius: 15
                                color: checked ? Theme.navyPrimary : Theme.bgTertiary
                                Behavior on color { ColorAnimation { duration: 150 } }

                                Rectangle {
                                    width: 26
                                    height: 26
                                    radius: 13
                                    color: "white"
                                    border.color: "#C0C0C0"
                                    border.width: 0.5
                                    x: enabledSwitch.checked ? enabledSwitch.width - width - 2 : 2
                                    y: 2
                                    Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: enabledSwitch.checked = !enabledSwitch.checked
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
                        height: qualityRow.height + 16
                        color: "transparent"

                        RowLayout {
                            id: qualityRow
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: 16

                            ColumnLayout {
                                Layout.preferredWidth: 260
                                spacing: 2

                                Text {
                                    text: "Preview quality"
                                    font.family: Theme.fontPrimary
                                    font.weight: Font.Bold
                                    font.pixelSize: 13
                                    color: Theme.textPrimary
                                }

                                Text {
                                    text: "Quality of the live preview feed."
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                }
                            }

                            Item { Layout.fillWidth: true }

                            Row {
                                spacing: 0
                                Layout.alignment: Qt.AlignVCenter

                                Repeater {
                                    model: ["Low", "Medium", "High"]
                                    delegate: Rectangle {
                                        required property string modelData
                                        width: 72
                                        height: 30
                                        color: modelData === "High" ? Theme.navyPrimary : Theme.bgSecondary
                                        border.color: Theme.borderColor
                                        border.width: 1

                                        Text {
                                            anchors.centerIn: parent
                                            text: modelData
                                            font.family: Theme.fontPrimary
                                            font.pixelSize: 12
                                            font.weight: modelData === "High" ? Font.Bold : Font.Normal
                                            color: modelData === "High" ? "white" : Theme.textSecondary
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
                        height: encoderRow.height + 16
                        color: "transparent"

                        RowLayout {
                            id: encoderRow
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: 16

                            ColumnLayout {
                                Layout.preferredWidth: 260
                                spacing: 2

                                Text {
                                    text: "Encoder / Bitrate"
                                    font.family: Theme.fontPrimary
                                    font.weight: Font.Bold
                                    font.pixelSize: 13
                                    color: Theme.textPrimary
                                }

                                Text {
                                    text: "Current encoding settings (read-only)."
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                }
                            }

                            Item { Layout.fillWidth: true }

                            RowLayout {
                                spacing: 8

                                Rectangle {
                                    width: 80
                                    height: 30
                                    radius: 6
                                    color: Theme.bgSecondary
                                    border.color: Theme.borderColor
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "H.265"
                                        font.family: Theme.fontMono
                                        font.pixelSize: 12
                                        color: Theme.textSecondary
                                    }
                                }

                                Rectangle {
                                    width: 100
                                    height: 30
                                    radius: 6
                                    color: Theme.bgSecondary
                                    border.color: Theme.borderColor
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: "12.0 Mbps"
                                        font.family: Theme.fontMono
                                        font.pixelSize: 12
                                        color: Theme.textSecondary
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item { Layout.minimumHeight: 24 }
        }
    }
}
