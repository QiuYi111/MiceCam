import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string cameraName: "Unknown"
    property double fps: 0.0
    property int drops: 0
    property bool isRecording: false
    property int status: 0
    property bool contextMenuOpen: false
    property string elapsedText: "00:00"

    signal contextConfigure()
    signal contextFullscreen()
    signal contextMenuRequested(real globalX, real globalY)

    radius: 12
    color: "#1A1A1E"
    clip: true

    property int cardRadius: 12

    Rectangle {
        id: previewSurface
        anchors.fill: parent
        radius: root.cardRadius
        clip: true
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2C3040" }
            GradientStop { position: 0.3; color: "#252830" }
            GradientStop { position: 0.7; color: "#1E2028" }
            GradientStop { position: 1.0; color: "#181A22" }
        }

        Canvas {
            id: previewCanvas
            anchors.fill: parent

            function roundedRectPath(ctx, x, y, w, h, r) {
                ctx.beginPath()
                ctx.moveTo(x + r, y)
                ctx.lineTo(x + w - r, y)
                ctx.quadraticCurveTo(x + w, y, x + w, y + r)
                ctx.lineTo(x + w, y + h - r)
                ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h)
                ctx.lineTo(x + r, y + h)
                ctx.quadraticCurveTo(x, y + h, x, y + h - r)
                ctx.lineTo(x, y + r)
                ctx.quadraticCurveTo(x, y, x + r, y)
                ctx.closePath()
            }

            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)

                ctx.save()
                roundedRectPath(ctx, 0, 0, width, height, root.cardRadius)
                ctx.clip()

                var cellW = width / 32
                var cellH = height / 24
                for (var gx = 0; gx < 32; gx++) {
                    for (var gy = 0; gy < 24; gy++) {
                        var noise = Math.random() * 12
                        var base = 28 + noise
                        ctx.fillStyle = "rgb(" + Math.floor(base) + "," + Math.floor(base + 2) + "," + Math.floor(base + 6) + ")"
                        ctx.fillRect(gx * cellW, gy * cellH, cellW + 0.5, cellH + 0.5)
                    }
                }
                ctx.strokeStyle = "rgba(60, 70, 90, 0.3)"
                ctx.lineWidth = 0.5
                for (var lx = 0; lx <= 4; lx++) {
                    var px = lx * width / 4
                    ctx.beginPath()
                    ctx.moveTo(px, 0)
                    ctx.lineTo(px, height)
                    ctx.stroke()
                }
                for (var ly = 0; ly <= 3; ly++) {
                    var py = ly * height / 3
                    ctx.beginPath()
                    ctx.moveTo(0, py)
                    ctx.lineTo(width, py)
                    ctx.stroke()
                }
                ctx.strokeStyle = "rgba(80, 100, 140, 0.12)"
                ctx.lineWidth = 1
                ctx.beginPath()
                ctx.moveTo(width * 0.33, height * 0.25)
                ctx.lineTo(width * 0.67, height * 0.25)
                ctx.lineTo(width * 0.67, height * 0.75)
                ctx.lineTo(width * 0.33, height * 0.75)
                ctx.closePath()
                ctx.stroke()
                var grd = ctx.createRadialGradient(width / 2, height / 2, width * 0.15, width / 2, height / 2, width * 0.55)
                grd.addColorStop(0, "rgba(40, 50, 70, 0.0)")
                grd.addColorStop(1, "rgba(10, 10, 15, 0.4)")
                ctx.fillStyle = grd
                ctx.fillRect(0, 0, width, height)

                ctx.restore()
            }
            Component.onCompleted: requestPaint()
        }

        Text {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 36
            anchors.rightMargin: 10
            text: root.elapsedText
            color: "#99FFFFFF"
            font.family: Theme.fontMono
            font.pixelSize: 11
            font.weight: Font.Medium
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: root.cardRadius
        border.width: root.status === 1 ? 2 : 0
        border.color: root.status === 1 ? Theme.statusAmber : "transparent"
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 10
        anchors.leftMargin: 10
        width: camLabel.implicitWidth + 14
        height: camLabel.implicitHeight + 6
        radius: 4
        color: "#80000000"

        Text {
            id: camLabel
            anchors.centerIn: parent
            text: root.cameraName
            color: "white"
            font.family: Theme.fontPrimary
            font.pixelSize: 13
            font.weight: Font.Bold
        }
    }

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.rightMargin: 10
        spacing: 5
        visible: root.isRecording

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
            font.pixelSize: 10
            font.weight: Font.Bold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    Rectangle {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        radius: 0
        color: "#B2000000"

        Canvas {
            id: bottomBarCorners
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d")
                ctx.clearRect(0, 0, width, height)
                ctx.save()
                var r = root.cardRadius
                ctx.beginPath()
                ctx.moveTo(0, 0)
                ctx.lineTo(width, 0)
                ctx.lineTo(width, height - r)
                ctx.quadraticCurveTo(width, height, width - r, height)
                ctx.lineTo(r, height)
                ctx.quadraticCurveTo(0, height, 0, height - r)
                ctx.closePath()
                ctx.clip()
                ctx.fillStyle = "#B2000000"
                ctx.fillRect(0, 0, width, height)
                ctx.restore()
            }
            Component.onCompleted: requestPaint()
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            Text {
                text: root.fps.toFixed(2) + " fps"
                color: "white"
                font.family: Theme.fontPrimary
                font.pixelSize: 11
                font.weight: Font.Medium
            }

            Item { Layout.fillWidth: true }

            Row {
                spacing: 6

                Text {
                    text: root.drops + " drops"
                    color: root.status === 1 ? Theme.statusAmber : "#CCCCCC"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 11
                    font.weight: Font.Medium
                }

                Rectangle {
                    visible: root.status === 1
                    width: 8; height: 8; radius: 4
                    color: Theme.statusAmber
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    MouseArea {
        id: cardMouseArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        z: 100
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                var globalPos = root.mapToItem(null, mouse.x, mouse.y)
                root.contextMenuRequested(globalPos.x, globalPos.y)
            }
        }
    }
}
