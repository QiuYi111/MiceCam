import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Popup {
    id: root
    width: 320
    padding: 0
    background: Rectangle {
        color: "white"
        radius: 12
        border.color: Theme.bgTertiary
        // Shadow mock
        layer.enabled: true
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        
        RowLayout {
            Layout.margins: 16
            Text { text: "Alerts"; font.family: Theme.fontPrimary; font.pixelSize: 16; font.weight: Font.Bold }
            Item { Layout.fillWidth: true }
            Text { text: "Clear All"; color: Theme.textSecondary; font.pixelSize: 12 }
        }
        
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgTertiary }
        
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            clip: true
            model: [
                { type: 2, title: "High drop rate detected on CAM_D", time: "00:42:12", count: 1, icon: "alerts" },
                { type: 1, title: "Encoder fallback on USB-1", time: "00:41:48", count: 2, icon: "encoding" },
                { type: 1, title: "High drop rate detected on CAM_C", time: "00:41:05", count: 0, icon: "alerts" },
                { type: 0, title: "Camera disconnect recovered on CAM_B", time: "00:40:33", count: 0, icon: "camera" }
            ]
            
            delegate: Rectangle {
                width: parent.width
                height: 64
                color: "transparent"
                
                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12
                    
                    Rectangle {
                        width: 32; height: 32; radius: 16
                        color: modelData.type === 2 ? "#FEE2E2" : (modelData.type === 1 ? "#FEF3C7" : "#DCFCE7")
                        AppIcon { 
                            anchors.centerIn: parent
                            name: modelData.icon
                            size: 16
                            color: modelData.type === 2 ? Theme.statusRed : (modelData.type === 1 ? Theme.statusAmber : Theme.statusGreen)
                        }
                    }
                    
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: modelData.title; font.family: Theme.fontPrimary; font.pixelSize: 12; font.weight: Font.DemiBold; elide: Text.ElideRight }
                        Text { text: "System"; color: Theme.textSecondary; font.pixelSize: 10 }
                    }
                    
                    ColumnLayout {
                        spacing: 2
                        Text { text: modelData.time; font.pixelSize: 10; color: Theme.textSecondary }
                        Rectangle {
                            visible: modelData.count > 0
                            width: 16; height: 16; radius: 8; color: Theme.statusRed
                            Text { anchors.centerIn: parent; text: modelData.count; color: "white"; font.pixelSize: 10 }
                        }
                    }
                }
                
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.bgTertiary; anchors.leftMargin: 56 }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 40
            color: "transparent"
            Text { anchors.centerIn: parent; text: "Show All Alerts"; color: Theme.navyPrimary; font.family: Theme.fontPrimary; font.pixelSize: 12 }
        }
    }
}
