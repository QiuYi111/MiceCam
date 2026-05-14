import QtQuick
import QtQuick.Controls
import QtQuick.Window
import "../theme"

Rectangle {
    id: root
    height: 38
    color: Theme.bgPrimary
    radius: 16
    
    // Cover the bottom rounded corners to keep them sharp
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: parent.radius
        color: parent.color
    }
    
    // Bottom border
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.bgTertiary
    }
    
    // Drag handler for moving the window
    DragHandler {
        onActiveChanged: if (active && Window.window) Window.window.startSystemMove()
    }
    
    // Window controls (macOS style on left)
    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 16
        spacing: 8
        
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: "#FF5F56"
            MouseArea {
                anchors.fill: parent
                onClicked: Qt.quit()
            }
        }
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: "#FFBD2E"
            MouseArea {
                anchors.fill: parent
                onClicked: Window.window.showMinimized()
            }
        }
        Rectangle {
            width: 12
            height: 12
            radius: 6
            color: "#27C93F"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (Window.window.visibility === Window.Maximized)
                        Window.window.showNormal()
                    else
                        Window.window.showMaximized()
                }
            }
        }
    }
    
    Text {
        anchors.centerIn: parent
        text: "MiceCam v2"
        font.family: Theme.fontPrimary
        font.pixelSize: 13
        font.weight: Font.DemiBold
        color: Theme.textPrimary
    }
}
