import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "theme"
import "components"

ApplicationWindow {
    id: appRoot
    visible: true
    width: 1200
    height: 800
    title: "MiceCam v2"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    property int currentViewIndex: 0
    property string selectedCameraName: ""
    property double selectedCameraFps: 0.0
    property int selectedCameraDrops: 0
    property int selectedCameraStatus: 0
    property bool selectedCameraRecording: false
    property var selectedResolutionOptions: []
    property var selectedFramerateOptions: []
    property var selectedFormatOptions: []

    function openCameraDetail(data) {
        selectedCameraName = data.name || ""
        selectedCameraFps = data.fps !== undefined ? data.fps : 0.0
        selectedCameraDrops = data.drops !== undefined ? data.drops : 0
        selectedCameraRecording = data.isRecording !== undefined ? data.isRecording : false
        selectedCameraStatus = data.status !== undefined ? data.status : 0
        selectedResolutionOptions = data.resolutionOptions || []
        selectedFramerateOptions = data.framerateOptions || []
        selectedFormatOptions = data.formatOptions || []
        currentViewIndex = 5
    }

    Rectangle {
        id: windowRoot
        anchors.fill: parent
        color: Theme.bgPrimary
        radius: 16
        clip: true

        AppTitleBar {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
        }

        AppToolbar {
            id: toolbar
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            isRecording: appController.isRecording
            canStartRecording: appController.canStartRecording
            recordText: appController.recordButtonText
            alertModel: appController.alertModel
            elapsedText: appController.elapsedText

            onFullscreenClicked: {
                var idx = appController.cameraModel.rowCount() > 0 ? 0 : -1
                if (idx >= 0) {
                    fullscreenView.open(appController.cameraAt(idx))
                }
            }

            onRecordClicked: {
                if (appController.isRecording) {
                    appController.stopRecording()
                } else if (!appController.startRecording()) {
                    preflightModal.items = appController.preflightItems()
                    preflightModal.open()
                }
            }

            onPreflightTriggered: {
                preflightModal.items = appController.preflightItems()
                preflightModal.open()
            }

            onSettingsClicked: {
                currentViewIndex = 1
            }
        }

        AppSidebar {
            id: sidebar
            anchors.top: toolbar.bottom
            anchors.left: parent.left
            anchors.bottom: statusBar.top
            activeViewIndex: currentViewIndex

            onViewChanged: (index) => {
                currentViewIndex = index
            }

            onCameraSelected: function(name, status) {
                var idx = sidebar.cameraList.currentIndex
                if (idx >= 0) {
                    var row = appController.cameraAt(idx)
                    if (row && Object.keys(row).length > 0) {
                        row.resolutionOptions = row.resolutionLabels || []
                        row.framerateOptions = row.framerateLabels || []
                        row.formatOptions = row.formatLabels || []
                        appRoot.openCameraDetail(row)
                    }
                }
            }
        }

        AppStatusBar {
            id: statusBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            elapsedText: appController.elapsedText
            cameraCountText: appController.cameraCountText
            totalFramesText: appController.totalFramesText
            averageFpsText: appController.averageFpsText
            bytesWrittenText: appController.bytesWrittenText
            diskRemainingText: appController.diskRemainingText
            preflightMessage: appController.preflightMessage
            cameraCount: appController.cameraCount
            recording: appController.isRecording
        }

        StackLayout {
            id: stackLayout
            currentIndex: currentViewIndex
            anchors.top: toolbar.bottom
            anchors.left: sidebar.right
            anchors.right: parent.right
            anchors.bottom: statusBar.top

            CameraGridView {
                id: cameraGridPage
                onCardFullscreen: function(name, fps, drops, isRecording, status, resOpts, fpsOpts, fmtOpts) {
                    fullscreenView.open({name: name, fps: fps, drops: drops, isRecording: isRecording, status: status})
                }
                onCardConfigure: function(name, fps, drops, isRecording, status, resOpts, fpsOpts, fmtOpts) {
                    appRoot.openCameraDetail({name: name, fps: fps, drops: drops, isRecording: isRecording, status: status,
                        resolutionOptions: resOpts, framerateOptions: fpsOpts, formatOptions: fmtOpts})
                }
            }
            EncodingSettings {}
            AlertsSettings {}
            LoggingSettings {
                onNavigateBack: currentViewIndex = 0
            }
            AboutView {}

            CameraDetailView {
                cameraName: appRoot.selectedCameraName
                cameraFps: appRoot.selectedCameraFps
                cameraDrops: appRoot.selectedCameraDrops
                cameraStatus: appRoot.selectedCameraStatus
                cameraRecording: appRoot.selectedCameraRecording
                resolutionOptionsList: appRoot.selectedResolutionOptions
                framerateOptionsList: appRoot.selectedFramerateOptions
                formatOptionsList: appRoot.selectedFormatOptions
                elapsedText: appController.elapsedText

                onBackClicked: {
                    currentViewIndex = 0
                }

                onFullscreenClicked: function(name, fps, drops, isRecording, status) {
                    fullscreenView.open({name: name, fps: fps, drops: drops, isRecording: isRecording, status: status})
                }
            }
        }

        FullscreenCameraView {
            id: fullscreenView
            anchors.fill: parent
            onClosed: {
            }
        }

        PreflightModal {
            id: preflightModal
            anchors.fill: parent
            onAdjustSettings: {
                currentViewIndex = 1
            }
        }
    }
}
