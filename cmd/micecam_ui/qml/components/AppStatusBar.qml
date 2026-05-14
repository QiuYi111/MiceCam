import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    height: 48
    color: Theme.bgPrimary
    radius: 16
    
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
        
        property bool isRecording: true // Mock state
        
        StatusSegment {
            Layout.minimumWidth: 130
            Layout.preferredWidth: 130
            icon: "clock"
            text: "00:42:17"
            textColor: Theme.statusRed
            iconColor: Theme.statusRed
        }
        
        Divider {}
        
        StatusSegment {
            Layout.minimumWidth: 130
            Layout.preferredWidth: 130
            icon: "camera"
            text: "5 cameras"
        }
        
        Divider {}
        
        StatusSegment {
            Layout.minimumWidth: 150
            Layout.preferredWidth: 150
            icon: "film"
            text: "76,230 frames"
        }
        
        Divider {}
        
        StatusSegment {
            Layout.minimumWidth: 150
            Layout.preferredWidth: 150
            icon: "chart"
            text: "29.97 fps avg"
        }
        
        Item { Layout.fillWidth: true }
        
        StatusSegment {
            Layout.minimumWidth: 100
            Layout.preferredWidth: 100
            icon: "disk"
            text: "3.2 GB"
        }
        
        Divider {}
        
        StatusSegment {
            Layout.minimumWidth: 170
            Layout.preferredWidth: 170
            icon: "chart"
            text: "45% disk remaining"
            textColor: Theme.statusAmber
            iconColor: Theme.statusAmber
        }
    }
    
    component StatusSegment : RowLayout {
        property string icon: ""
        property string text: ""
        property color textColor: Theme.textPrimary
        property color iconColor: Theme.textPrimary
        Layout.fillHeight: true
        spacing: 8
        clip: false
        
        AppIcon {
            name: icon
            size: 16
            color: iconColor
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: text
            font.family: Theme.fontPrimary
            font.pixelSize: 13
            font.weight: Font.Medium
            color: textColor
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
