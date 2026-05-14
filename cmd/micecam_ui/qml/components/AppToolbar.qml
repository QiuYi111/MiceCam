import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root
    height: 56
    color: Theme.bgPrimary
    
    // Bottom border
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.bgTertiary
    }
    
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12
        
        // Record / Stop Button Group
        Rectangle {
            id: recordBtn
            width: isRecording ? 180 : 110
            height: 36
            radius: 8
            color: "transparent"
            border.color: Theme.recordRed
            border.width: 1
            clip: true
            
            property bool isRecording: true // Mock state for design matching
            
            RowLayout {
                anchors.fill: parent
                spacing: 0
                
                // Left side (Red button)
                Rectangle {
                    Layout.preferredWidth: recordBtn.isRecording ? 90 : recordBtn.width
                    Layout.fillHeight: true
                    color: Theme.recordRed
                    radius: 8
                    
                    // Only round right corners if not recording
                    layer.enabled: recordBtn.isRecording
                    
                    RowLayout {
                        anchors.centerIn: parent
                        spacing: 8
                        
                        // Icon (White circle with Red Square)
                        Rectangle {
                            width: 18; height: 18; radius: 9; color: "white"
                            Rectangle {
                                width: 8; height: 8; radius: 1; color: Theme.recordRed
                                anchors.centerIn: parent
                            }
                        }
                        
                        Text {
                            text: recordBtn.isRecording ? "Stop" : "Record"
                            font.family: "SF Pro Text"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            color: "white"
                        }
                    }
                }
                
                // Right side (Timer)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "white"
                    visible: recordBtn.isRecording
                    
                    Text {
                        anchors.centerIn: parent
                        text: "00:42:17"
                        font.family: "SF Pro Text"
                        font.pixelSize: 14
                        color: Theme.recordRed
                    }
                }
            }
        }
        
        Item { Layout.fillWidth: true }
        
        // Right side tools
        RowLayout {
            spacing: 12
            
            // Notifications
            Rectangle {
                width: 36; height: 36; radius: 8; color: "transparent"; border.color: Theme.bgTertiary; border.width: 1
                AppIcon { anchors.centerIn: parent; name: "alerts"; size: 18 }
                Rectangle {
                    anchors.top: parent.top; anchors.right: parent.right; anchors.topMargin: -4; anchors.rightMargin: -4
                    width: 18; height: 18; radius: 9; color: Theme.statusRed
                    Text { anchors.centerIn: parent; text: "3"; color: "white"; font.pixelSize: 11; font.weight: Font.Bold }
                }
                MouseArea { anchors.fill: parent; onClicked: notifyPopup.open() }
                NotificationPopup { id: notifyPopup; y: parent.height + 8; x: -width + parent.width }
            }
            
            // Fullscreen
            Rectangle {
                width: 36; height: 36; radius: 8; color: "transparent"; border.color: Theme.bgTertiary; border.width: 1
                AppIcon { anchors.centerIn: parent; name: "fullscreen"; size: 18 }
            }
            
            // Settings
            Rectangle {
                width: 110; height: 36; radius: 8; color: "transparent"; border.color: Theme.bgTertiary; border.width: 1
                RowLayout {
                    anchors.centerIn: parent; spacing: 8
                    AppIcon { name: "gear"; size: 16 }
                    Text { text: "Settings"; font.family: "SF Pro Text"; font.pixelSize: 14; color: Theme.textPrimary }
                    AppIcon { name: "chevron-right"; size: 8; color: Theme.textSecondary; rotation: 90 }
                }
                MouseArea { anchors.fill: parent; onClicked: root.parent.currentViewIndex = 1 }
            }
        }
    }
}
