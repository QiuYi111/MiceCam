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

    property int selectedResolutionIndex: 0
    property int selectedFrameRateIndex: 1
    property int selectedStreamModeIndex: 1

    readonly property string selectedResolution: resolutionOptions.get(selectedResolutionIndex).label
    readonly property string selectedFrameRate: frameRateOptions.get(selectedFrameRateIndex).label
    readonly property string selectedPixelFormat: streamModeOptions.get(selectedStreamModeIndex).label

    ListModel {
        id: resolutionOptions
        ListElement { label: "1920\u00d71080"; value: "1920x1080" }
        ListElement { label: "1280\u00d7720"; value: "1280x720" }
        ListElement { label: "640\u00d7480"; value: "640x480" }
    }

    ListModel {
        id: frameRateOptions
        ListElement { label: "15 fps"; value: "15" }
        ListElement { label: "30 fps"; value: "30" }
        ListElement { label: "60 fps"; value: "60" }
    }

    ListModel {
        id: streamModeOptions
        ListElement { label: "Mono8"; value: "Mono8" }
        ListElement { label: "BGR"; value: "BGR" }
        ListElement { label: "NV12"; value: "NV12" }
    }

    signal backClicked()
    signal fullscreenClicked(string name, real fps, int drops, bool isRecording, int status)

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: detailContent.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: detailContent
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 32
            anchors.rightMargin: 32
            spacing: 20

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
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

                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 260
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
                            text: root.selectedFrameRate
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
                            text: root.selectedResolution
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
                implicitHeight: metricsGrid.implicitHeight + 32
                color: "white"
                radius: 10
                border.color: Theme.borderColor
                border.width: 1

                GridLayout {
                    id: metricsGrid
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 16
                    columns: 4
                    rowSpacing: 12
                    columnSpacing: 16

                    Repeater {
                        model: [
                            { label: "Resolution", value: root.selectedResolution },
                            { label: "Frame Rate", value: root.selectedFrameRate },
                            { label: "Pixel Format", value: root.selectedPixelFormat },
                            { label: "Encoder", value: "H.265 (HEVC)" },
                            { label: "Bitrate", value: "12.0 Mbps" },
                            { label: "Frame Drops", value: root.cameraDrops.toString() },
                            { label: "Buffer", value: "48/64" },
                            { label: "Uptime", value: "00:42:17" }
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

            Text {
                text: "Acquisition Configuration"
                font.family: Theme.fontPrimary
                font.weight: Font.Bold
                font.pixelSize: 16
                color: Theme.textPrimary
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: acqInner.implicitHeight + 24
                color: "white"
                radius: 10
                border.color: Theme.borderColor
                border.width: 1

                ColumnLayout {
                    id: acqInner
                    anchors.fill: parent
                    anchors.topMargin: 12
                    anchors.bottomMargin: 12
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        height: resolutionRow.height + 20
                        color: "transparent"

                        RowLayout {
                            id: resolutionRow
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            spacing: 16

                            ColumnLayout {
                                Layout.preferredWidth: 200
                                spacing: 2

                                Text {
                                    text: "Resolution"
                                    font.family: Theme.fontPrimary
                                    font.weight: Font.Bold
                                    font.pixelSize: 13
                                    color: Theme.textPrimary
                                }

                                Text {
                                    text: "Camera output resolution."
                                    font.family: Theme.fontPrimary
                                    font.pixelSize: 12
                                    color: Theme.textSecondary
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                }
                            }

                            Item { Layout.fillWidth: true }

            ComboBox {
                id: resolutionCombo
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 180
                Layout.rightMargin: 16
                model: resolutionOptions
                textRole: "label"
                currentIndex: root.selectedResolutionIndex
                onActivated: root.selectedResolutionIndex = currentIndex

                delegate: ItemDelegate {
                    required property string label
                    required property string value
                    width: resolutionCombo.width
                    contentItem: Text {
                        text: label
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: highlighted ? "white" : Theme.textPrimary
                        verticalAlignment: Text.AlignVCenter
                    }
                    highlighted: resolutionCombo.highlightedIndex === index
                    background: Rectangle {
                        color: highlighted ? Theme.navyPrimary : "white"
                    }
                }

                contentItem: Text {
                    text: resolutionOptions.get(resolutionCombo.currentIndex).label
                    font.family: Theme.fontMono
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textPrimary
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }

                background: Rectangle {
                    radius: 6
                    color: "white"
                    border.color: resolutionCombo.activeFocus ? Theme.navyPrimary : Theme.borderColor
                    border.width: 1
                    implicitHeight: 34
                }

                indicator: Canvas {
                    x: resolutionCombo.width - width - 10
                    y: resolutionCombo.height / 2 - height / 2
                    width: 10
                    height: 6
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.fillStyle = Theme.textSecondary
                        ctx.beginPath()
                        ctx.moveTo(0, 0)
                        ctx.lineTo(width, 0)
                        ctx.lineTo(width / 2, height)
                        ctx.closePath()
                        ctx.fill()
                    }
                    Component.onCompleted: requestPaint()
                }

                popup: Popup {
                    y: resolutionCombo.height
                    width: resolutionCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: resolutionCombo.popup.visible ? resolutionCombo.delegateModel : null
                        currentIndex: resolutionCombo.highlightedIndex
                    }

                    background: Rectangle {
                        radius: 6
                        color: "white"
                        border.color: Theme.borderColor
                        border.width: 1
                        layer.enabled: true
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
        height: fpsRow.height + 20
        color: "transparent"

        RowLayout {
            id: fpsRow
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 16

            ColumnLayout {
                Layout.preferredWidth: 200
                spacing: 2

                Text {
                    text: "Frame Rate"
                    font.family: Theme.fontPrimary
                    font.weight: Font.Bold
                    font.pixelSize: 13
                    color: Theme.textPrimary
                }

                Text {
                    text: "Target acquisition frame rate."
                    font.family: Theme.fontPrimary
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }

            Item { Layout.fillWidth: true }

            ComboBox {
                id: frameRateCombo
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 160
                Layout.rightMargin: 16
                model: frameRateOptions
                textRole: "label"
                currentIndex: root.selectedFrameRateIndex
                onActivated: root.selectedFrameRateIndex = currentIndex

                delegate: ItemDelegate {
                    required property string label
                    required property string value
                    width: frameRateCombo.width
                    contentItem: Text {
                        text: label
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: highlighted ? "white" : Theme.textPrimary
                        verticalAlignment: Text.AlignVCenter
                    }
                    highlighted: frameRateCombo.highlightedIndex === index
                    background: Rectangle {
                        color: highlighted ? Theme.navyPrimary : "white"
                    }
                }

                contentItem: Text {
                    text: frameRateOptions.get(frameRateCombo.currentIndex).label
                    font.family: Theme.fontMono
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textPrimary
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }

                background: Rectangle {
                    radius: 6
                    color: "white"
                    border.color: frameRateCombo.activeFocus ? Theme.navyPrimary : Theme.borderColor
                    border.width: 1
                    implicitHeight: 34
                }

                indicator: Canvas {
                    x: frameRateCombo.width - width - 10
                    y: frameRateCombo.height / 2 - height / 2
                    width: 10
                    height: 6
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.fillStyle = Theme.textSecondary
                        ctx.beginPath()
                        ctx.moveTo(0, 0)
                        ctx.lineTo(width, 0)
                        ctx.lineTo(width / 2, height)
                        ctx.closePath()
                        ctx.fill()
                    }
                    Component.onCompleted: requestPaint()
                }

                popup: Popup {
                    y: frameRateCombo.height
                    width: frameRateCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: frameRateCombo.popup.visible ? frameRateCombo.delegateModel : null
                        currentIndex: frameRateCombo.highlightedIndex
                    }

                    background: Rectangle {
                        radius: 6
                        color: "white"
                        border.color: Theme.borderColor
                        border.width: 1
                        layer.enabled: true
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
        height: streamModeRow.height + 20
        color: "transparent"

        RowLayout {
            id: streamModeRow
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 16

            ColumnLayout {
                Layout.preferredWidth: 200
                spacing: 2

                Text {
                    text: "Stream Mode"
                    font.family: Theme.fontPrimary
                    font.weight: Font.Bold
                    font.pixelSize: 13
                    color: Theme.textPrimary
                }

                Text {
                    text: "Pixel format / stream mode."
                    font.family: Theme.fontPrimary
                    font.pixelSize: 12
                    color: Theme.textSecondary
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }

            Item { Layout.fillWidth: true }

            ComboBox {
                id: streamModeCombo
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: 160
                Layout.rightMargin: 16
                model: streamModeOptions
                textRole: "label"
                currentIndex: root.selectedStreamModeIndex
                onActivated: root.selectedStreamModeIndex = currentIndex

                delegate: ItemDelegate {
                    required property string label
                    required property string value
                    width: streamModeCombo.width
                    contentItem: Text {
                        text: label
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: highlighted ? "white" : Theme.textPrimary
                        verticalAlignment: Text.AlignVCenter
                    }
                    highlighted: streamModeCombo.highlightedIndex === index
                    background: Rectangle {
                        color: highlighted ? Theme.navyPrimary : "white"
                    }
                }

                contentItem: Text {
                    text: streamModeOptions.get(streamModeCombo.currentIndex).label
                    font.family: Theme.fontMono
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textPrimary
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: 10
                }

                background: Rectangle {
                    radius: 6
                    color: "white"
                    border.color: streamModeCombo.activeFocus ? Theme.navyPrimary : Theme.borderColor
                    border.width: 1
                    implicitHeight: 34
                }

                indicator: Canvas {
                    x: streamModeCombo.width - width - 10
                    y: streamModeCombo.height / 2 - height / 2
                    width: 10
                    height: 6
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.clearRect(0, 0, width, height)
                        ctx.fillStyle = Theme.textSecondary
                        ctx.beginPath()
                        ctx.moveTo(0, 0)
                        ctx.lineTo(width, 0)
                        ctx.lineTo(width / 2, height)
                        ctx.closePath()
                        ctx.fill()
                    }
                    Component.onCompleted: requestPaint()
                }

                popup: Popup {
                    y: streamModeCombo.height
                    width: streamModeCombo.width
                    implicitHeight: contentItem.implicitHeight
                    padding: 1

                    contentItem: ListView {
                        clip: true
                        implicitHeight: contentHeight
                        model: streamModeCombo.popup.visible ? streamModeCombo.delegateModel : null
                        currentIndex: streamModeCombo.highlightedIndex
                    }

                    background: Rectangle {
                        radius: 6
                        color: "white"
                        border.color: Theme.borderColor
                        border.width: 1
                        layer.enabled: true
                    }
                }
            }
        }
    }
                }
            }

            Text {
                text: "Recording & Preview"
                font.family: Theme.fontPrimary
                font.weight: Font.Bold
                font.pixelSize: 16
                color: Theme.textPrimary
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: recInner.implicitHeight + 24
                color: "white"
                radius: 10
                border.color: Theme.borderColor
                border.width: 1

                ColumnLayout {
                    id: recInner
                    anchors.fill: parent
                    anchors.topMargin: 12
                    anchors.bottomMargin: 12
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16
                        height: enabledRow.height + 20
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
                                height: 32
                                radius: 16
                                color: checked ? Theme.navyPrimary : Theme.bgTertiary
                                Behavior on color { ColorAnimation { duration: 150 } }

                                Rectangle {
                                    width: 26
                                    height: 26
                                    radius: 13
                                    color: "white"
                                    border.color: "#C0C0C0"
                                    border.width: 0.5
                                    x: enabledSwitch.checked ? enabledSwitch.width - width - 3 : 3
                                    y: 3
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
                        height: qualityRow.height + 20
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
                                        height: 32
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
                        height: encoderRow.height + 20
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
                                    height: 32
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
                                    height: 32
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

            Item { Layout.minimumHeight: 32 }
        }
    }
}
