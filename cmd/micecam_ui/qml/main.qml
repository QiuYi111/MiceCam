import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "theme"
import "components"

ApplicationWindow {
    visible: true
    width: 1200
    height: 800
    title: "MiceCam v2"
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    property int currentViewIndex: 0

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
        }

        AppSidebar {
            id: sidebar
            anchors.top: toolbar.bottom
            anchors.left: parent.left
            anchors.bottom: statusBar.top

            onViewChanged: (index) => {
                currentViewIndex = index
            }
        }

        AppStatusBar {
            id: statusBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
        }

        StackLayout {
            currentIndex: currentViewIndex
            anchors.top: toolbar.bottom
            anchors.left: sidebar.right
            anchors.right: parent.right
            anchors.bottom: statusBar.top

            CameraGridView {}
            EncodingSettings {}
            AlertsSettings {}
            LoggingSettings {}
            AboutView {}
        }
    }
}
