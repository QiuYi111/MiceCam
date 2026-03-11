import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform 1.1 as Platform
import "components"
import "theme/Theme.js" as Theme

ApplicationWindow {
    id: mainWindow

    visible: true
    width: 1360
    height: 880
    minimumWidth: 1100
    minimumHeight: 760
    title: qsTr("MiceCam")
    color: Theme.appBackground

    property var cameraOptions: []
    property var resolutionOptions: []
    property bool compactLayout: width < 1220
    property bool dockCompact: width < 1040
    property bool showValidationHints: false
    property real parsedFps: Number(fpsField.text)
    property bool fpsValid: isFinite(parsedFps) && parsedFps > 0 && parsedFps <= 240
    property bool sessionNameValid: sessionField.text.trim().length > 0
    property bool outputDirValid: outputField.text.trim().length > 0
    property bool cameraSelected: cameraCombo.currentIndex >= 0 && cameraOptions.length > 0
    property bool resolutionValid: resolutionCombo.currentIndex >= 0 && resolutionCombo.currentText.length > 0
    property bool readyToRecord: cameraSelected
                                  && sessionNameValid
                                  && outputDirValid
                                  && fpsValid
                                  && resolutionValid
                                  && !pipeline.isRecording
                                  && !pipeline.isDecoding

    function summaryText() {
        if (pipeline.isDecoding) {
            return "Exporting to " + pipeline.resolvedExportPath
        }
        if (pipeline.isRecording) {
            return "Recording into " + pipeline.resolvedSessionPath
        }
        return outputField.text.trim().length > 0 ? outputField.text.trim() : "Choose an output folder"
    }

    function currentCameraLabel() {
        if (!cameraOptions || cameraOptions.length === 0) {
            return "No camera detected"
        }
        if (cameraCombo.currentIndex < 0 || cameraCombo.currentIndex >= cameraOptions.length) {
            return cameraOptions[0].name
        }
        return cameraOptions[cameraCombo.currentIndex].name
    }

    function currentResolutionLabel() {
        if (!resolutionOptions || resolutionOptions.length === 0) {
            return "No resolution"
        }
        if (resolutionCombo.currentIndex < 0 || resolutionCombo.currentIndex >= resolutionOptions.length) {
            return resolutionOptions[0]
        }
        return resolutionOptions[resolutionCombo.currentIndex]
    }

    function refreshCameraOptions() {
        cameraOptions = pipeline.getAvailableCameras()

        if (cameraOptions.length === 0) {
            if (typeof cameraCombo !== "undefined" && cameraCombo) {
                cameraCombo.currentIndex = -1
            }
            resolutionOptions = []
            return
        }

        if (typeof cameraCombo !== "undefined" && cameraCombo
                && (cameraCombo.currentIndex < 0 || cameraCombo.currentIndex >= cameraOptions.length)) {
            cameraCombo.currentIndex = 0
        }

        syncResolutions()
    }

    function syncResolutions() {
        if (!cameraOptions || cameraOptions.length === 0 || typeof cameraCombo === "undefined" || !cameraCombo || cameraCombo.currentIndex < 0) {
            resolutionOptions = []
            return
        }

        resolutionOptions = pipeline.getAvailableResolutions(cameraOptions[cameraCombo.currentIndex].id)
        resolutionCombo.currentIndex = resolutionOptions.length > 0 ? 0 : -1
    }

    function startCapture() {
        if (!readyToRecord) {
            showValidationHints = true
            return
        }

        showValidationHints = false
        var selectedCamera = cameraOptions[cameraCombo.currentIndex]
        var parts = resolutionCombo.currentText.split("x")
        if (parts.length !== 2) {
            return
        }

        pipeline.sessionName = sessionField.text.trim()
        pipeline.outputDir = outputField.text.trim()
        pipeline.autoDecode = autoDecodeCheck.checked
        pipeline.startRecording(
            selectedCamera.id,
            selectedCamera.type,
            parseInt(parts[0]),
            parseInt(parts[1]),
            parsedFps
        )
    }

    Component.onCompleted: {
        sessionField.text = pipeline.sessionName
        outputField.text = pipeline.outputDir
        autoDecodeCheck.checked = pipeline.autoDecode
        refreshCameraOptions()
    }

    Connections {
        target: pipeline

        function onSessionNameChanged(name) {
            sessionField.text = name
        }

        function onOutputDirChanged(dir) {
            outputField.text = dir
        }

        function onAutoDecodeChanged(autoDecode) {
            autoDecodeCheck.checked = autoDecode
        }
    }

    background: Rectangle {
        color: Theme.appBackground
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        AppHeader {
            Layout.fillWidth: true
            compact: dockCompact
            sessionState: pipeline.sessionState
            statusHeadline: pipeline.statusHeadline
            statusDetail: pipeline.statusDetail
            sessionName: pipeline.sessionName
            outputDir: outputField.text
        }

        Rectangle {
            Layout.fillWidth: true
            visible: pipeline.lastErrorMessage.length > 0
            color: Theme.errorSoft
            border.color: Theme.error
            border.width: 1
            radius: Theme.radiusControl
            implicitHeight: errorText.implicitHeight + Theme.space16 * 2

            Text {
                id: errorText
                anchors.fill: parent
                anchors.margins: Theme.space16
                text: pipeline.lastErrorMessage
                color: Theme.error
                font.pixelSize: 13
                wrapMode: Text.Wrap
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: compactLayout ? stackedWorkspace : desktopWorkspace
        }

        ActionDock {
            Layout.fillWidth: true
            compact: dockCompact
            readyToRecord: readyToRecord
            busy: pipeline.isDecoding
            sessionState: pipeline.sessionState
            outputSummary: summaryText()
            autoDecode: autoDecodeCheck.checked
            primaryText: pipeline.isRecording ? "Stop Recording" :
                         pipeline.isDecoding ? "Exporting" : "Start Recording"
            secondaryText: pipeline.isRecording ? "" : "Browse Output"

            onPrimaryClicked: {
                if (pipeline.isRecording) {
                    pipeline.stopRecording()
                } else {
                    startCapture()
                }
            }

            onSecondaryClicked: folderDialog.open()
        }
    }

    Component {
        id: desktopWorkspace

        RowLayout {
            spacing: Theme.space20

            PreviewStage {
                Layout.fillWidth: true
                Layout.fillHeight: true
                isRecording: pipeline.isRecording
                isDecoding: pipeline.isDecoding
                sessionState: pipeline.sessionState
                headline: pipeline.statusHeadline
                detail: pipeline.statusDetail
                elapsedText: pipeline.recordingDurationText
                fpsValue: pipeline.currentFps
                throughputValue: pipeline.mbps
                droppedFrames: pipeline.droppedFrames
                hasCamera: pipeline.hasAvailableCamera
            }

            ScrollView {
                Layout.preferredWidth: 332
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Loader {
                    id: desktopRailLoader
                    Layout.fillWidth: true
                    width: 332
                    sourceComponent: railContent
                    onLoaded: refreshCameraOptions()
                }
            }
        }
    }

    Component {
        id: stackedWorkspace

        ScrollView {
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: mainWindow.width - 36
                spacing: 14

                PreviewStage {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 400
                    isRecording: pipeline.isRecording
                    isDecoding: pipeline.isDecoding
                    sessionState: pipeline.sessionState
                    headline: pipeline.statusHeadline
                    detail: pipeline.statusDetail
                    elapsedText: pipeline.recordingDurationText
                    fpsValue: pipeline.currentFps
                    throughputValue: pipeline.mbps
                    droppedFrames: pipeline.droppedFrames
                    hasCamera: pipeline.hasAvailableCamera
                }

                Loader {
                    id: stackedRailLoader
                    Layout.fillWidth: true
                    sourceComponent: railContent
                    onLoaded: refreshCameraOptions()
                }
            }
        }
    }

    Component {
        id: railContent

        ColumnLayout {
            width: parent ? parent.width : 0
            spacing: Theme.space16

            Card {
                Layout.fillWidth: true
                title: pipeline.isRecording || pipeline.isDecoding ? "Current Session" : "Session Setup"
                subtitle: pipeline.isRecording || pipeline.isDecoding
                    ? "Capture settings are locked while work is in progress."
                    : "Source, naming, and export behavior."

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space12

                    Text {
                        text: "Camera"
                        color: Theme.textSecondary
                        font.pixelSize: 12
                    }

                    ComboBox {
                        id: cameraCombo
                        Layout.fillWidth: true
                        implicitHeight: 40
                        model: cameraOptions
                        textRole: "name"
                        valueRole: "id"
                        enabled: !pipeline.isRecording && !pipeline.isDecoding

                        onCurrentIndexChanged: syncResolutions()
                        Component.onCompleted: refreshCameraOptions()
                        onModelChanged: {
                            if (cameraOptions.length > 0 && currentIndex < 0) {
                                currentIndex = 0
                            }
                        }

                        contentItem: Text {
                            leftPadding: Theme.space16
                            rightPadding: Theme.space32
                            text: currentCameraLabel()
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        indicator: Canvas {
                            x: cameraCombo.width - width - Theme.space16
                            y: cameraCombo.topPadding + (cameraCombo.availableHeight - height) / 2
                            width: 12
                            height: 8

                            onPaint: {
                                var ctx = getContext("2d")
                                ctx.clearRect(0, 0, width, height)
                                ctx.moveTo(0, 0)
                                ctx.lineTo(width, 0)
                                ctx.lineTo(width / 2, height)
                                ctx.closePath()
                                ctx.fillStyle = Theme.textSecondary
                                ctx.fill()
                            }
                        }

                        background: Rectangle {
                            radius: Theme.radiusControl
                            color: Theme.surface
                            border.color: Theme.borderSubtle
                        }
                    }

                    Text {
                        text: "Session Name"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }

                    TextField {
                        id: sessionField
                        Layout.fillWidth: true
                        implicitHeight: 40
                        enabled: !pipeline.isRecording && !pipeline.isDecoding
                        color: Theme.textPrimary
                        font.pixelSize: 13
                        placeholderText: "Session name"
                        placeholderTextColor: Theme.textTertiary
                        selectByMouse: true
                        onTextEdited: pipeline.sessionName = text.trim()

                        background: Rectangle {
                            radius: Theme.radiusControl
                            color: Theme.surface
                            border.color: showValidationHints && !sessionNameValid ? Theme.error : Theme.borderSubtle
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space12

                            Text {
                                text: "Resolution"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }

                            ComboBox {
                                id: resolutionCombo
                                Layout.fillWidth: true
                                implicitHeight: 40
                                model: resolutionOptions
                                enabled: !pipeline.isRecording && !pipeline.isDecoding
                                onModelChanged: {
                                    if (resolutionOptions.length > 0 && currentIndex < 0) {
                                        currentIndex = 0
                                    }
                                }

                                contentItem: Text {
                                    leftPadding: Theme.space16
                                    rightPadding: Theme.space32
                                    text: currentResolutionLabel()
                                    color: Theme.textPrimary
                                    font.pixelSize: 13
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                indicator: Canvas {
                                    x: resolutionCombo.width - width - Theme.space16
                                    y: resolutionCombo.topPadding + (resolutionCombo.availableHeight - height) / 2
                                    width: 12
                                    height: 8

                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0, 0, width, height)
                                        ctx.moveTo(0, 0)
                                        ctx.lineTo(width, 0)
                                        ctx.lineTo(width / 2, height)
                                        ctx.closePath()
                                        ctx.fillStyle = Theme.textSecondary
                                        ctx.fill()
                                    }
                                }

                                background: Rectangle {
                                    radius: Theme.radiusControl
                                    color: Theme.surface
                                    border.color: showValidationHints && !resolutionValid ? Theme.error : Theme.borderSubtle
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.preferredWidth: 104
                            spacing: Theme.space12

                            Text {
                                text: "FPS"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }

                            TextField {
                                id: fpsField
                                Layout.fillWidth: true
                                implicitHeight: 40
                                text: "30"
                                enabled: !pipeline.isRecording && !pipeline.isDecoding
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                horizontalAlignment: Text.AlignHCenter
                                validator: DoubleValidator {
                                    bottom: 1
                                    top: 240
                                }

                                background: Rectangle {
                                    radius: Theme.radiusControl
                                    color: Theme.surface
                                    border.color: showValidationHints && !fpsValid ? Theme.error : Theme.borderSubtle
                                }
                            }
                        }
                    }

                    Text {
                        text: "Output Folder"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.space12

                        TextField {
                            id: outputField
                            Layout.fillWidth: true
                            implicitHeight: 40
                            enabled: !pipeline.isRecording && !pipeline.isDecoding
                            color: Theme.textPrimary
                            font.pixelSize: 13
                            placeholderText: "recordings"
                            placeholderTextColor: Theme.textTertiary
                            selectByMouse: true
                            onTextEdited: pipeline.outputDir = text.trim()

                            background: Rectangle {
                                radius: Theme.radiusControl
                                color: Theme.surface
                                border.color: showValidationHints && !outputDirValid ? Theme.error : Theme.borderSubtle
                            }
                        }

                        SecondaryButton {
                            text: "Browse"
                            enabled: !pipeline.isRecording && !pipeline.isDecoding
                            onClicked: folderDialog.open()
                        }
                    }

                    CheckBox {
                        id: autoDecodeCheck
                        text: "Prepare export automatically after recording"
                        enabled: !pipeline.isRecording
                        checked: true
                        onToggled: pipeline.autoDecode = checked

                        indicator: Rectangle {
                            implicitWidth: 20
                            implicitHeight: 20
                            radius: 6
                            x: autoDecodeCheck.leftPadding
                            y: parent.height / 2 - height / 2
                            color: autoDecodeCheck.checked ? Theme.accent : Theme.surface
                            border.color: autoDecodeCheck.checked ? Theme.accent : Theme.borderSubtle

                            Rectangle {
                                anchors.centerIn: parent
                                width: 8
                                height: 8
                                radius: 4
                                color: "white"
                                visible: autoDecodeCheck.checked
                            }
                        }

                        contentItem: Text {
                            text: autoDecodeCheck.text
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            leftPadding: autoDecodeCheck.indicator.width + Theme.space12
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.Wrap
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        visible: showValidationHints && !readyToRecord && !pipeline.isRecording && !pipeline.isDecoding
                        color: Theme.surface
                        radius: Theme.radiusControl
                        border.color: Theme.errorSoft
                        implicitHeight: readinessText.implicitHeight + Theme.space12 * 2

                        Text {
                            id: readinessText
                            anchors.fill: parent
                            anchors.margins: Theme.space12
                            text: !pipeline.hasAvailableCamera ? "Connect a camera to begin."
                                  : !sessionNameValid ? "Enter a session name."
                                  : !resolutionValid ? "Choose a resolution."
                                  : !fpsValid ? "Use an FPS value between 1 and 240."
                                  : !outputDirValid ? "Choose an output folder."
                                  : "Ready to record."
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: pipeline.isRecording ? "System" : "Readiness"
                subtitle: pipeline.isRecording ? "Live health and throughput." : "Camera, output, and capture status."

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    StatusRow {
                        Layout.fillWidth: true
                        dotColor: pipeline.hasAvailableCamera ? Theme.success : Theme.warning
                        label: pipeline.hasAvailableCamera ? "Camera available" : "No camera available"
                    }

                    StatusRow {
                        Layout.fillWidth: true
                        dotColor: outputDirValid ? Theme.success : Theme.warning
                        label: outputDirValid ? "Output path ready" : "Output path needed"
                    }

                    StatusRow {
                        Layout.fillWidth: true
                        dotColor: pipeline.hasDroppedFramesWarning ? Theme.error : Theme.success
                        label: pipeline.hasDroppedFramesWarning ? "Dropped frames detected" : "No dropped frames"
                        value: pipeline.hasDroppedFramesWarning ? String(pipeline.droppedFrames) : ""
                    }

                    StatusRow {
                        Layout.fillWidth: true
                        dotColor: pipeline.isDecoding ? Theme.warning : Theme.success
                        label: pipeline.isDecoding ? "Export in progress" : "Capture engine idle"
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.topMargin: 4
                        visible: pipeline.isRecording || pipeline.isDecoding || pipeline.sessionState === "completed"
                        color: Theme.surface
                        radius: 12
                        border.color: Theme.separator
                        implicitHeight: 78

                        GridLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            columns: 2
                            rowSpacing: 4
                            columnSpacing: 16

                            Text {
                                text: "FPS"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }

                            Text {
                                text: Number(pipeline.currentFps).toFixed(1)
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "Throughput"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }

                            Text {
                                text: Number(pipeline.mbps).toFixed(2) + " Mbps"
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignRight
                                Layout.fillWidth: true
                            }

                            Text {
                                text: "Duration"
                                color: Theme.textSecondary
                                font.pixelSize: 11
                            }

                            Text {
                                text: pipeline.recordingDurationText
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                horizontalAlignment: Text.AlignRight
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: "Live Metrics"
                subtitle: "Performance stays secondary until recording begins."
                visible: pipeline.isRecording

                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    rowSpacing: Theme.space12
                    columnSpacing: Theme.space12

                    MetricTile {
                        Layout.fillWidth: true
                        label: "FPS"
                        value: Number(pipeline.currentFps).toFixed(1)
                        accentColor: Theme.accent
                    }

                    MetricTile {
                        Layout.fillWidth: true
                        label: "Duration"
                        value: pipeline.recordingDurationText
                        accentColor: Theme.textPrimary
                    }

                    MetricTile {
                        Layout.fillWidth: true
                        label: "Frames"
                        value: String(pipeline.capturedFrames)
                        accentColor: Theme.textPrimary
                    }

                    MetricTile {
                        Layout.fillWidth: true
                        label: "Dropped"
                        value: String(pipeline.droppedFrames)
                        accentColor: pipeline.hasDroppedFramesWarning ? Theme.error : Theme.textPrimary
                    }

                    MetricTile {
                        Layout.fillWidth: true
                        label: "Throughput"
                        value: Number(pipeline.mbps).toFixed(2)
                        accentColor: Theme.accent
                    }

                    MetricTile {
                        Layout.fillWidth: true
                        label: "Format"
                        value: pipeline.format
                        accentColor: Theme.textPrimary
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: pipeline.isDecoding ? "Export Progress" : "Session Paths"
                subtitle: pipeline.isDecoding
                    ? "Post-processing stays explicit instead of hiding behind a thin bar."
                    : "Keep the raw session and export destinations visible."
                visible: pipeline.isDecoding || pipeline.sessionState === "completed" || pipeline.sessionState === "error"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space10

                    Text {
                        text: "Session path"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }

                    Text {
                        text: pipeline.resolvedSessionPath
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        wrapMode: Text.WrapAnywhere
                    }

                    Text {
                        text: "Export path"
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }

                    Text {
                        text: pipeline.resolvedExportPath
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        wrapMode: Text.WrapAnywhere
                    }

                    ProgressBar {
                        id: decodeBar
                        Layout.fillWidth: true
                        visible: pipeline.isDecoding || pipeline.decodeProgress > 0
                        from: 0
                        to: 100
                        value: pipeline.decodeProgress

                        background: Rectangle {
                            implicitHeight: 8
                            radius: 4
                            color: Theme.surfaceMuted
                        }

                        contentItem: Item {
                            implicitHeight: 8

                            Rectangle {
                                width: decodeBar.visualPosition * parent.width
                                height: parent.height
                                radius: 4
                                color: Theme.accent
                            }
                        }
                    }
                }
            }

            Card {
                Layout.fillWidth: true
                title: "Recent Activity"
                subtitle: "Most recent backend events."

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.space10

                    Repeater {
                        model: Math.min(pipeline.logMessages.length, 5)

                        delegate: Rectangle {
                            Layout.fillWidth: true
                            color: Theme.surface
                            radius: Theme.radiusControl
                            border.color: Theme.separator
                            implicitHeight: logText.implicitHeight + Theme.space12 * 2

                            Text {
                                id: logText
                                anchors.fill: parent
                                anchors.margins: Theme.space12
                                text: pipeline.logMessages[pipeline.logMessages.length - index - 1]
                                color: Theme.textSecondary
                                font.pixelSize: 11
                                wrapMode: Text.WrapAnywhere
                            }
                        }
                    }

                    Text {
                        visible: pipeline.logMessages.length === 0
                        text: "No activity yet."
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }
            }
        }
    }

    Platform.FolderDialog {
        id: folderDialog
        title: "Choose Output Folder"

        onAccepted: {
            var selectedFolder = String(folder).replace("file://", "")
            outputField.text = selectedFolder
            pipeline.outputDir = selectedFolder
        }
    }
}
