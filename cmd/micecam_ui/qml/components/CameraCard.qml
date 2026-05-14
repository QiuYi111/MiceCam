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
    property int status: 0 // 0: Normal, 1: Warning, 2: Error
    
    radius: 12
    color: Theme.navyDark
    clip: true
    
    // Placeholder for video content
    Rectangle {
        anchors.fill: parent
        color: "#2A2A2E"
        
        // Mock video image (mouse in a cage)
        Image {
            anchors.fill: parent
            source: "https://raw.githubusercontent.com/QiuYi111/MiceCam/v2/assets/mock_cam.png" // Fallback or mock
            fillMode: Image.PreserveAspectCrop
            opacity: 0.8
        }
        
        Text {
            anchors.centerIn: parent
            text: "Video Preview"
            color: Theme.textTertiary
            font.family: "SF Pro Text"
            visible: false
        }
    }
    
    // Top Info
    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 12
        
        Text {
            text: root.cameraName
            color: "white"
            font.family: "SF Pro Text"
            font.pixelSize: 14
            font.weight: Font.Bold
        }
        
        Item { Layout.fillWidth: true }
        
        Row {
            spacing: 4
            visible: root.isRecording
            Rectangle {
                width: 8; height: 8; radius: 4
                color: Theme.recordRed
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "REC"
                color: "white"
                font.family: "SF Pro Text"
                font.pixelSize: 10
                font.weight: Font.Bold
            }
        }
    }
    
    // Bottom Overlay
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 28
        color: "#B2000000" // 70% black
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            
            Text {
                text: root.fps.toFixed(2) + " fps"
                color: "white"
                font.family: "SF Pro Text"
                font.pixelSize: 12
                font.weight: Font.Medium
            }
            
            Item { Layout.fillWidth: true }
            
            Row {
                spacing: 4
                Text {
                    text: root.drops + " drops"
                    color: root.status === 1 ? Theme.statusAmber : "white"
                    font.family: "SF Pro Text"
                    font.pixelSize: 12
                    font.weight: Font.Medium
                }
                Text {
                    visible: root.status === 1
                    text: "⚠️"
                    font.pixelSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
    
    // Border for selection/status
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: root.status === 1 ? 2 : 0
        border.color: Theme.statusAmber
        radius: 12
    }
}
