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
    property bool recording: false
    
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
            Layout.minimumWidth: 130
            Layout.preferredWidth: 130
            icon: "clock"
            labelText: root.elapsedText
            textColor: root.recording ? Theme.statusRed : Theme.textPrimary
            iconColor: root.recording ? Theme.statusRed : Theme.textPrimary
        }

        Divider {}

        StatusSegment {
            Layout.minimumWidth: 130
            Layout.preferredWidth: 130
            icon: "camera"
            labelText: root.cameraCountText
        }

        Divider {}

        StatusSegment {
            Layout.minimumWidth: 150
            Layout.preferredWidth: 150
            icon: "film"
            labelText: root.totalFramesText
        }

        Divider {}

        StatusSegment {
            Layout.minimumWidth: 150
            Layout.preferredWidth: 150
            icon: "chart"
            labelText: root.averageFpsText
        }

        Item { Layout.fillWidth: true }

        StatusSegment {
            Layout.minimumWidth: 100
            Layout.preferredWidth: 100
            icon: "disk"
            labelText: root.bytesWrittenText
        }

        Divider {}

        StatusSegment {
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
