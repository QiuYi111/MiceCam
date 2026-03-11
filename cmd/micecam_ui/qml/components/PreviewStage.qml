import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

Rectangle {
    id: root

    property bool isRecording: false
    property bool isDecoding: false
    property string sessionState: "idle"
    property string headline: ""
    property string detail: ""
    property string elapsedText: "00:00"
    property real fpsValue: 0
    property real throughputValue: 0
    property int droppedFrames: 0
    property bool hasCamera: true

    radius: Theme.radiusStage
    border.color: Theme.borderSubtle
    border.width: 1
    clip: true

    color: root.isRecording ? "#0e1718" : Theme.surfaceRaised

    Image {
        id: previewImage
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        asynchronous: false
        cache: false
        source: "image://live_camera/feed?t=" + frameTicker.tick
        visible: root.isRecording
    }

    Timer {
        id: frameTicker
        property int tick: 0
        interval: 70
        running: root.isRecording
        repeat: true
        onTriggered: tick += 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            StateChip {
                text: root.sessionState === "recording" ? "Recording" :
                      root.sessionState === "decoding" ? "Decoding" :
                      root.sessionState === "completed" ? "Completed" :
                      root.sessionState === "error" ? "Needs attention" : "Ready"
                state: root.sessionState
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: root.isRecording
                radius: Theme.radiusPill
                color: "#ffffff22"
                implicitHeight: 32
                implicitWidth: elapsedLabel.implicitWidth + Theme.space16 * 2

                Text {
                    id: elapsedLabel
                    anchors.centerIn: parent
                    text: root.elapsedText
                    color: "white"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }

        Item { Layout.fillHeight: true }

        Item {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 220
            Layout.preferredHeight: 220
            visible: !root.isRecording

            Rectangle {
                anchors.centerIn: parent
                width: 220
                height: 220
                radius: 110
                color: Theme.surface
                border.color: Theme.separator
                border.width: 1
            }

            Rectangle {
                anchors.centerIn: parent
                width: 146
                height: 146
                radius: 73
                color: Theme.surfaceMuted
            }

            Rectangle {
                anchors.centerIn: parent
                width: 66
                height: 66
                radius: 33
                color: Theme.accentSoft
            }

            Rectangle {
                anchors.centerIn: parent
                width: 18
                height: 18
                radius: 9
                color: Theme.accent
            }
        }

        Item { Layout.fillHeight: true }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6
            visible: !root.isRecording

            Text {
                text: root.sessionState === "decoding" ? "Preparing export" :
                      root.sessionState === "completed" ? "Session complete" :
                      !root.hasCamera ? "Connect a camera" : "Live preview"
                color: Theme.textPrimary
                font.pixelSize: 22
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
            }

            Text {
                text: root.detail
                color: Theme.textSecondary
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                Layout.maximumWidth: 320
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            visible: root.isRecording || root.droppedFrames > 0

            Rectangle {
                visible: root.isRecording
                radius: Theme.radiusPill
                color: "#ffffff20"
                implicitHeight: 34
                implicitWidth: fpsText.implicitWidth + Theme.space16 * 2

                Text {
                    id: fpsText
                    anchors.centerIn: parent
                    text: "FPS " + Number(root.fpsValue).toFixed(1)
                    color: "white"
                    font.pixelSize: 12
                }
            }

            Rectangle {
                visible: root.isRecording
                radius: Theme.radiusPill
                color: "#ffffff20"
                implicitHeight: 34
                implicitWidth: mbpsText.implicitWidth + Theme.space16 * 2

                Text {
                    id: mbpsText
                    anchors.centerIn: parent
                    text: Number(root.throughputValue).toFixed(2) + " Mbps"
                    color: "white"
                    font.pixelSize: 12
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: root.droppedFrames > 0
                radius: Theme.radiusPill
                color: "#9a4b46dd"
                implicitHeight: 34
                implicitWidth: warningText.implicitWidth + Theme.space16 * 2

                Text {
                    id: warningText
                    anchors.centerIn: parent
                    text: root.droppedFrames + " dropped"
                    color: "white"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
            }
        }
    }
}
