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
            Layout.preferredWidth: 120
            icon: "clock"
            text: "00:42:17"
            textColor: Theme.statusRed
            iconColor: Theme.statusRed
        }
        
        Divider {}
        
        StatusSegment {
            Layout.preferredWidth: 120
            icon: "camera"
            text: "5 cameras"
        }
        
        Divider {}
        
        StatusSegment {
            Layout.preferredWidth: 140
            icon: "film"
            text: "76,230 frames"
        }
        
        Divider {}
        
        StatusSegment {
            Layout.preferredWidth: 140
            icon: "chart"
            text: "29.97 fps avg"
        }
        
        Item { Layout.fillWidth: true } // Spacer
        
        StatusSegment {
            Layout.preferredWidth: 100
            icon: "disk"
            text: "3.2 GB"
        }
        
        Divider {}
        
        StatusSegment {
            Layout.preferredWidth: 160
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
        Layout.preferredWidth: 160
        Layout.fillHeight: true
        spacing: 12
        
        AppIcon {
            name: icon
            size: 16
            color: iconColor
            Layout.alignment: Qt.AlignVCenter
        }
        Text {
            text: text
            font.family: "SF Pro Text"
            font.pixelSize: 13
            font.weight: Font.Medium
            color: textColor
            Layout.alignment: Qt.AlignVCenter
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
