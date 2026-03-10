import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform 1.1 as Platform

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 600
    height: 900
    title: qsTr("MiceCam Pro (Native C++)")
    color: "#f0f2f5"

    Rectangle {
        id: header
        width: parent.width
        height: 80
        color: "#1a4b8c"
        z: 10

        RowLayout {
            anchors.fill: parent
            anchors.margins: 15

            Text {
                text: "🐭"
                font.pixelSize: 36
                Layout.alignment: Qt.AlignVCenter
            }

            ColumnLayout {
                spacing: 2
                Layout.alignment: Qt.AlignVCenter
                Text {
                    text: "MiceCam Pro"
                    color: "white"
                    font.pixelSize: 22
                    font.bold: true
                }
                Text {
                    text: "Native Behavior Recording System"
                    color: Qt.rgba(1, 1, 1, 0.8)
                    font.pixelSize: 13
                }
            }
            Item { Layout.fillWidth: true }
        }
    }

    ScrollView {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true

        ColumnLayout {
            width: mainWindow.width - 40
            x: 20
            spacing: 15

            GroupBox {
                title: "📹 Source & Output"
                Layout.fillWidth: true
                Layout.topMargin: 15

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    spacing: 10

                    Text { text: "Camera Device"; color: "#5f6368" }
                    ComboBox {
                        id: cameraCombo
                        Layout.fillWidth: true
                        model: pipeline.getAvailableCameras()
                        textRole: "name"
                        valueRole: "id"
                        enabled: !pipeline.isRecording
                        onActivated: {
                            resolutionCombo.model = pipeline.getAvailableResolutions(currentValue);
                        }
                    }

                    Text { text: "Session Name"; color: "#5f6368" }
                    TextField {
                        id: sessionField
                        Layout.fillWidth: true
                        text: pipeline.sessionName
                        enabled: !pipeline.isRecording
                        onTextChanged: if (!pipeline.isRecording) pipeline.sessionName = text
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        ColumnLayout {
                            Layout.fillWidth: true
                            Text { text: "Resolution"; color: "#5f6368" }
                            ComboBox {
                                id: resolutionCombo
                                Layout.fillWidth: true
                                enabled: !pipeline.isRecording
                            }
                        }
                        ColumnLayout {
                            Layout.preferredWidth: 80
                            Text { text: "FPS"; color: "#5f6368" }
                            TextField {
                                id: fpsField
                                text: "30"
                                Layout.fillWidth: true
                                enabled: !pipeline.isRecording
                            }
                        }
                    }

                    Text { text: "Output Directory"; color: "#5f6368" }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: outputField
                            Layout.fillWidth: true
                            text: pipeline.outputDir
                            enabled: !pipeline.isRecording
                            onTextChanged: if (!pipeline.isRecording) pipeline.outputDir = text
                        }
                        Button {
                            text: "Browse..."
                            enabled: !pipeline.isRecording
                            onClicked: folderDialog.open()
                        }
                    }
                }
            }

            GroupBox {
                title: "📊 Statistics"
                Layout.fillWidth: true

                GridLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    columns: 3
                    rowSpacing: 10
                    columnSpacing: 10

                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: pipeline.currentFps.toFixed(1); font.pixelSize: 22; font.bold: true; color: "#1a4b8c"; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "FPS"; font.pixelSize: 11; color: "#5f6368"; Layout.alignment: Qt.AlignHCenter }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: pipeline.capturedFrames; font.pixelSize: 22; font.bold: true; color: "#1a4b8c"; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "Frames"; font.pixelSize: 11; color: "#5f6368"; Layout.alignment: Qt.AlignHCenter }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: pipeline.droppedFrames; font.pixelSize: 22; font.bold: true; color: "#1a4b8c"; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "Dropped"; font.pixelSize: 11; color: "#5f6368"; Layout.alignment: Qt.AlignHCenter }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: pipeline.mbps.toFixed(2); font.pixelSize: 22; font.bold: true; color: "#1a4b8c"; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "MB/s"; font.pixelSize: 11; color: "#5f6368"; Layout.alignment: Qt.AlignHCenter }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        Text { text: pipeline.format; font.pixelSize: 22; font.bold: true; color: "#1a4b8c"; Layout.alignment: Qt.AlignHCenter }
                        Text { text: "Format"; font.pixelSize: 11; color: "#5f6368"; Layout.alignment: Qt.AlignHCenter }
                    }
                }
            }

            GroupBox {
                title: "⚡ Controls"
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    spacing: 15

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 15

                        Button {
                            id: startBtn
                            text: "▶ Start Recording"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 45
                            font.pixelSize: 16
                            font.bold: true
                            enabled: !pipeline.isRecording

                            background: Rectangle {
                                color: parent.enabled ? (parent.down ? "#143d72" : "#1a4b8c") : "#dadce0"
                                radius: 6
                            }
                            contentItem: Text {
                                text: parent.text;
                                color: parent.enabled ? "white" : "#bdc1c6"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            onClicked: {
                                var backend = cameraCombo.currentValue
                                var type = cameraCombo.model[cameraCombo.currentIndex].type
                                var res = resolutionCombo.currentText.split('x')
                                var w = parseInt(res[0])
                                var h = parseInt(res[1])
                                var f = parseFloat(fpsField.text)
                                pipeline.startRecording(backend, type, w, h, f)
                            }
                        }

                        Button {
                            id: stopBtn
                            text: "⏹ Stop"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 45
                            font.pixelSize: 16
                            font.bold: true
                            enabled: pipeline.isRecording

                            background: Rectangle {
                                color: parent.enabled ? (parent.down ? "#b31412" : "#d93025") : "#fce8e6"
                                radius: 6
                            }
                            contentItem: Text {
                                text: parent.text;
                                color: parent.enabled ? "white" : "#ea4335"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: pipeline.stopRecording()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        CheckBox {
                            id: autoDecodeCheck
                            text: "Auto-Decode session"
                            checked: pipeline.autoDecode
                            onCheckedChanged: pipeline.autoDecode = checked
                        }
                        ProgressBar {
                            Layout.fillWidth: true
                            value: pipeline.decodeProgress / 100.0
                            visible: pipeline.decodeProgress > 0 && pipeline.decodeProgress < 100
                        }
                    }
                }
            }

            GroupBox {
                title: "👁 Live Preview"
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: 10

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.minimumHeight: 300
                        color: "black"
                        radius: 4

                        Image {
                            id: previewImage
                            anchors.fill: parent
                            cache: false
                            asynchronous: false
                            fillMode: Image.PreserveAspectFit
                            visible: pipeline.isRecording
                            source: "image://live_camera/feed?t=" + previewTimer.count
                        }

                        Timer {
                            id: previewTimer
                            property int count: 0
                            interval: 50
                            running: pipeline.isRecording
                            repeat: true
                            onTriggered: count++
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "Preview Offline"
                            color: "#7f8c8d"
                            visible: !pipeline.isRecording
                        }
                    }
                }
            }

            GroupBox {
                title: "📜 System Log"
                Layout.fillWidth: true
                Layout.preferredHeight: 200

                background: Rectangle {
                    color: "#f8f9fa"
                    border.color: "#e8eaed"
                    radius: 6
                    y: parent.topPadding - 5
                    width: parent.width
                    height: parent.height - parent.topPadding + 5
                }

                ListView {
                    id: logList
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true
                    model: pipeline.logMessages
                    delegate: Text {
                        text: modelData
                        font.family: "Monospace"
                        font.pixelSize: 11
                        color: "#3c4043"
                        width: logList.width
                        wrapMode: Text.WrapAnywhere
                    }
                    onCountChanged: logList.positionViewAtEnd()
                }
            }

            Item { Layout.minimumHeight: 20 }
        }
    }

    Platform.FolderDialog {
        id: folderDialog
        title: "Choose Output Directory"
        onAccepted: pipeline.outputDir = String(folder).replace("file://", "")
    }

    Component.onCompleted: {
        if (cameraCombo.count > 0) {
            resolutionCombo.model = pipeline.getAvailableResolutions(cameraCombo.currentValue)
            resolutionCombo.currentIndex = 0
        }
    }
}
