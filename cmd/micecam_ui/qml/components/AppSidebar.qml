import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MiceCam.Models
import "../theme"

Rectangle {
    id: root
    width: 240
    color: Theme.bgSecondary
    
    signal viewChanged(int index)
    
    // Right border
    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.bgTertiary
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 0
        
        Text {
            text: "Cameras"
            font.family: "SF Pro Text"
            font.pixelSize: 13
            color: Theme.textSecondary
            Layout.bottomMargin: 12
            Layout.leftMargin: 8
        }
        
        ListView {
            id: cameraList
            Layout.fillWidth: true
            Layout.preferredHeight: contentHeight
            model: CameraModel {}
            interactive: false
            
            delegate: Rectangle {
                id: cameraItem
                width: ListView.view.width
                height: 40
                radius: 8
                color: (root.parent.currentViewIndex === 0 && index === 0) ? "#E5E7EB" : "transparent"
                
                // Selection indicator (Left)
                Rectangle {
                    width: 4
                    height: 24
                    radius: 2
                    color: Theme.navyPrimary
                    visible: (root.parent.currentViewIndex === 0 && index === 0)
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 12
                    spacing: 12
                    
                    // Camera Icon
                    Item {
                        width: 18; height: 18
                        Rectangle {
                            width: 12; height: 10; radius: 2; color: Theme.textPrimary
                            anchors.centerIn: parent
                            anchors.horizontalCenterOffset: -2
                        }
                        // Triangle part
                        Canvas {
                            anchors.fill: parent
                            onPaint: {
                                var ctx = getContext("2d");
                                ctx.fillStyle = Theme.textPrimary;
                                ctx.beginPath();
                                ctx.moveTo(12, 6); ctx.lineTo(16, 4); ctx.lineTo(16, 14); ctx.lineTo(12, 12);
                                ctx.closePath(); ctx.fill();
                            }
                        }
                    }
                    
                    Text {
                        text: model.name
                        font.family: "SF Pro Text"
                        font.pixelSize: 14
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                    }
                    
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: model.status === 0 ? Theme.statusGreen : (model.status === 1 ? Theme.statusAmber : Theme.statusRed)
                    }
                }
                
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.viewChanged(0)
                }
            }
        }
        
        Item { Layout.fillHeight: true }
        
        // Navigation Section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            
            Repeater {
                model: [
                    { name: "Encoding", icon: "encoding" },
                    { name: "Alerts", icon: "alerts" },
                    { name: "Output", icon: "logging" },
                    { name: "About", icon: "about" }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    radius: 8
                    color: root.parent.currentViewIndex === (index + 1) ? Theme.navyLight : "transparent"
                    
                    // Selection indicator (Left)
                    Rectangle {
                        width: 4; height: 24; radius: 2; color: Theme.navyPrimary
                        visible: root.parent.currentViewIndex === (index + 1)
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 12
                        
                        AppIcon {
                            name: modelData.icon
                            size: 16
                            color: root.parent.currentViewIndex === (index + 1) ? Theme.navyPrimary : Theme.textPrimary
                        }
                        
                        Text {
                            text: modelData.name
                            font.family: "SF Pro Text"
                            font.pixelSize: 14
                            color: root.parent.currentViewIndex === (index + 1) ? Theme.navyPrimary : Theme.textPrimary
                            Layout.fillWidth: true
                        }
                        
                        AppIcon {
                            name: "chevron-right"
                            size: 10
                            color: Theme.textTertiary
                            visible: root.parent.currentViewIndex !== (index + 1)
                        }
                    }
                    
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.viewChanged(index + 1)
                    }
                }
            }
        }
    }
}
