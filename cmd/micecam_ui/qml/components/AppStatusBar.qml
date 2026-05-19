import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    height: 48
    color: Theme.bgPrimary
    radius: 16

    property string elapsedText: "00:00:00"
    property string cameraCountText: "0 cameras"
    property string totalFramesText: "0 frames"
    property string averageFpsText: "0.00 fps avg"
    property string bytesWrittenText: "0 B"
    property string diskRemainingText: "Disk unknown"
    property string preflightMessage: "Ready"
    property int cameraCount: 0
    property bool recording: false
    readonly property bool readyToRecord: !recording && cameraCount > 0
    readonly property bool missingCameras: !recording && cameraCount === 0
    
    // Cover the top rounded corners to keep them sharp
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: parent.radius
        color: parent.color
    }
    
    // Top border
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.bgTertiary
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        spacing: 0
        
        StatusSegment {
            Layout.minimumWidth: 150
            Layout.preferredWidth: 150
            icon: root.recording ? "clock" : (root.readyToRecord ? "check" : "warning")
            labelText: root.recording ? root.elapsedText : (root.readyToRecord ? "Ready to start" : "Preflight required")
            textColor: root.recording ? Theme.statusRed : (root.missingCameras ? Theme.statusAmber : Theme.textPrimary)
            iconColor: root.recording ? Theme.statusRed : (root.missingCameras ? Theme.statusAmber : Theme.textPrimary)
        }

        Divider {}

        StatusSegment {
            Layout.minimumWidth: 130
            Layout.preferredWidth: 130
            icon: "camera"
            labelText: root.cameraCountText
            textColor: root.cameraCount === 0 ? Theme.statusAmber : Theme.textPrimary
            iconColor: root.cameraCount === 0 ? Theme.statusAmber : Theme.textPrimary
        }

        Divider {}

        StatusSegment {
            Layout.minimumWidth: 260
            Layout.preferredWidth: 320
            icon: root.recording ? "film" : "warning"
            labelText: root.recording ? root.totalFramesText : root.preflightMessage
            textColor: root.recording ? Theme.textPrimary : (root.missingCameras ? Theme.statusAmber : Theme.textSecondary)
            iconColor: root.recording ? Theme.textPrimary : (root.missingCameras ? Theme.statusAmber : Theme.textSecondary)
        }

        Item { Layout.fillWidth: true }

        StatusSegment {
            visible: root.recording
            Layout.minimumWidth: 150
            Layout.preferredWidth: 150
            icon: "chart"
            labelText: root.averageFpsText
        }

        Divider {
            visible: root.recording
        }

        StatusSegment {
            Layout.minimumWidth: root.recording ? 110 : 190
            Layout.preferredWidth: root.recording ? 110 : 190
            icon: "disk"
            labelText: root.recording ? root.bytesWrittenText : "No session output"
            textColor: root.recording ? Theme.textPrimary : Theme.textSecondary
            iconColor: root.recording ? Theme.textPrimary : Theme.textSecondary
        }

        Divider {
            visible: root.recording
        }

        StatusSegment {
            visible: root.recording
            Layout.minimumWidth: 170
            Layout.preferredWidth: 170
            icon: "chart"
            labelText: root.diskRemainingText
            textColor: Theme.statusAmber
            iconColor: Theme.statusAmber
        }
    }
    
    component StatusSegment : RowLayout {
        id: segment
        property string icon: ""
        property string labelText: ""
        property color textColor: Theme.textPrimary
        property color iconColor: Theme.textPrimary
        Layout.fillHeight: true
        spacing: 8
        clip: false

        AppIcon {
            name: segment.icon
            size: 16
            color: segment.iconColor
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: segment.labelText
            font.family: Theme.fontPrimary
            font.pixelSize: 13
            font.weight: Font.Medium
            color: segment.textColor
            Layout.alignment: Qt.AlignVCenter
            clip: false
        }
    }
    
    component Divider : Rectangle {
        width: 1
        height: 20
        color: Theme.bgTertiary
        Layout.alignment: Qt.AlignVCenter
        Layout.leftMargin: 20
        Layout.rightMargin: 20
    }
}
