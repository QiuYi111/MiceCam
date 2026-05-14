import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    anchors.fill: parent
    color: Theme.overlayBg
    visible: false
    z: 100

    property string cameraName: "CAM_A"
    property double fps: 29.97
    property int drops: 0
    property bool isRecording: true
    property int status: 0

    signal closed()

    function open(name, f, d, rec, s) {
        cameraName = name || "CAM_A"
        fps = f !== undefined ? f : 29.97
        drops = d !== undefined ? d : 0
        isRecording = rec !== undefined ? rec : true
        status = s !== undefined ? s : 0
        visible = true
    }

    function close() {
        visible = false
        root.closed()
    }

    Keys.onEscapePressed: root.close()

    Rectangle {
        id: contentArea
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 960)
        height: Math.min(parent.height - 80, 640)
        radius: 12
        color: "#1A1A1E"
        clip: true

        Rectangle {
            id: topControls
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 48
            color: "#B2000000"
            z: 2

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16
                spacing: 12

                Text {
                    text: root.cameraName
                    font.family: Theme.fontPrimary
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: "white"
                }

                Row {
                    visible: root.isRecording
                    spacing: 6
                    Layout.alignment: Qt.AlignVCenter

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: Theme.recordRed
                        anchors.verticalCenter: parent.verticalCenter

                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                            NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                        }
                    }

                    Text {
                        text: "REC"
                        color: "white"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 11
                        font.weight: Font.Bold
                        Layout.alignment: Qt.AlignVCenter
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "00:42:17"
                    font.family: Theme.fontMono
                    font.pixelSize: 13
                    color: "#99FFFFFF"
                }

                Rectangle {
                    width: 32; height: 32; radius: 16
                    color: "#40FFFFFF"

                    Text {
                        anchors.centerIn: parent
                        text: "X"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 14
                        font.weight: Font.Bold
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }
        }

        Canvas {
            id: fullscreenPreview
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                var cellW = width / 48
                var cellH = height / 36
                for (var gx = 0; gx < 48; gx++) {
                    for (var gy = 0; gy < 36; gy++) {
                        var noise = Math.random() * 12
                        var base = 28 + noise
                        ctx.fillStyle = "rgb(" + Math.floor(base) + "," + Math.floor(base + 2) + "," + Math.floor(base + 6) + ")"
                        ctx.fillRect(gx * cellW, gy * cellH, cellW + 0.5, cellH + 0.5)
                    }
                }
                ctx.strokeStyle = "rgba(60, 70, 90, 0.2)"
                ctx.lineWidth = 0.5
                for (var lx = 0; lx <= 8; lx++) {
                    var px = lx * width / 8
                    ctx.beginPath(); ctx.moveTo(px, 0); ctx.lineTo(px, height); ctx.stroke()
                }
                for (var ly = 0; ly <= 6; ly++) {
                    var py = ly * height / 6
                    ctx.beginPath(); ctx.moveTo(0, py); ctx.lineTo(width, py); ctx.stroke()
                }
                ctx.strokeStyle = "rgba(80, 100, 140, 0.08)"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(width * 0.33, height * 0.25)
                ctx.lineTo(width * 0.67, height * 0.25)
                ctx.lineTo(width * 0.67, height * 0.75)
                ctx.lineTo(width * 0.33, height * 0.75)
                ctx.closePath()
                ctx.stroke()
            }
            Component.onCompleted: requestPaint()
        }

        Rectangle {
            id: bottomMetrics
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 40
            color: "#B2000000"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Text {
                    text: root.fps.toFixed(2) + " fps"
                    color: "white"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                }

                Item { Layout.fillWidth: true }

                Row {
                    spacing: 16

                    Text {
                        text: root.drops + " drops"
                        color: root.status === 1 ? Theme.statusAmber : "#CCCCCC"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                    }

                    Rectangle {
                        visible: root.status === 1
                        width: 8; height: 8; radius: 4
                        color: Theme.statusAmber
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Text {
                        text: "H.265 / 1080p"
                        color: "#CCCCCC"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 12
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: function(mouse) {
            if (!contentArea.contains(contentArea.mapFromItem(root, mouse.x, mouse.y))) {
                root.close()
            }
        }
    }
}
