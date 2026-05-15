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
    property double selectedCameraFps: 29.97
    property int selectedCameraDrops: 0
    property int selectedCameraStatus: 0
    property bool selectedCameraRecording: true

    function openCameraDetail(name, fps, drops, isRecording, status) {
        selectedCameraName = name
        selectedCameraFps = fps !== undefined ? fps : 29.97
        selectedCameraDrops = drops !== undefined ? drops : 0
        selectedCameraRecording = isRecording !== undefined ? isRecording : true
        selectedCameraStatus = status !== undefined ? status : 0
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
            recordText: appController.recordButtonText
            alertModel: appController.alertModel

            onFullscreenClicked: {
                fullscreenView.open("CAM_A", 29.97, 0, true, 0)
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
                var fps = 29.97
                var drops = 0
                var rec = true
                var st = status
                if (name === "CAM_D") { fps = 18.45; drops = 152; }
                appRoot.openCameraDetail(name, fps, drops, rec, st)
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
                onCardFullscreen: function(name, fps, drops, isRecording, status) {
                    fullscreenView.open(name, fps, drops, isRecording, status)
                }
                onCardConfigure: function(name, fps, drops, isRecording, status) {
                    appRoot.openCameraDetail(name, fps, drops, isRecording, status)
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

                onBackClicked: {
                    currentViewIndex = 0
                }

                onFullscreenClicked: function(name, fps, drops, isRecording, status) {
                    fullscreenView.open(name, fps, drops, isRecording, status)
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
