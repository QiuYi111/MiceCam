import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Flickable {
    id: root
    contentHeight: mainCol.height
    clip: true
    
    ColumnLayout {
        id: mainCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 32
        spacing: 24
        
        // Header
        RowLayout {
            spacing: 12
            Text {
                text: "<  Cameras"
                font.family: "SF Pro Text"
                font.pixelSize: 14
                color: Theme.navyPrimary
                MouseArea { anchors.fill: parent; onClicked: root.parent.parent.currentViewIndex = 0 }
            }
        }
        
        RowLayout {
            Text { text: "Alerts"; font.family: "SF Pro Text"; font.pixelSize: 28; font.weight: Font.Bold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            Row {
                spacing: 8
                Text { text: "✓"; color: Theme.statusGreen; font.pixelSize: 16 }
                Text { text: "All changes saved automatically"; color: Theme.textSecondary; font.family: "SF Pro Text"; font.pixelSize: 12 }
            }
        }
        
        // Settings List
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: listCol.implicitHeight + 32
            color: "white"
            radius: 12
            border.color: Theme.bgTertiary
            
            ColumnLayout {
                id: listCol
                anchors.fill: parent
                anchors.margins: 16
                spacing: 0
                
                // Feishu Webhook
                SettingRow {
                    title: "Feishu webhook URL"
                    description: "Send alert notifications to a Feishu group via incoming webhook."
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        TextField { 
                            id: webhookField
                            Layout.fillWidth: true
                            text: "••••••••••••••••••••••••••••••••••••••••••••" 
                            echoMode: TextInput.Password
                            font.pixelSize: 13
                            leftPadding: 12
                            rightPadding: 40
                            background: Rectangle { 
                                border.color: Theme.bgTertiary; border.width: 1; radius: 8; color: "white" 
                                AppIcon { 
                                    name: "eye"; size: 16; color: Theme.textSecondary
                                    anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter
                                }
                            }
                        }
                    }
                    Text { text: "Your webhook URL is stored securely and never shared."; font.pixelSize: 11; color: Theme.textTertiary; Layout.topMargin: -4 }
                }
                
                Divider {}
                
                // Watchdog
                SettingRow {
                    title: "Watchdog timeout"
                    description: "Trigger an alert if a camera stops sending frames for the specified time."
                    RowLayout {
                        spacing: 12
                        Rectangle {
                            width: 144; height: 36
                            radius: 8
                            border.color: Theme.bgTertiar
                            border.width: 1
                            clip: true
                            Row {
                                anchors.fill: parent
                                spacing: 0
                                Rectangle { width: 36; height: 36; color: "#F9FAFB"; 
                                    Text { anchors.centerIn: parent; text: "—"; font.pixelSize: 12; color: Theme.textPrimary }
                                }
                                Rectangle { width: 1; height: 36; color: Theme.bgTertiary }
                                Rectangle { width: 70; height: 36; color: "white"; 
                                    Text { anchors.centerIn: parent; text: "3"; font.family: "SF Pro Text"; font.pixelSize: 14; font.weight: Font.Bold; color: Theme.textPrimary }
                                }
                                Rectangle { width: 1; height: 36; color: Theme.bgTertiary }
                                Rectangle { width: 36; height: 36; color: "#F9FAFB"; 
                                    Text { anchors.centerIn: parent; text: "+"; font.pixelSize: 16; color: Theme.textPrimary }
                                }
                            }
                        }
                        Text { text: "seconds"; font.pixelSize: 13; color: Theme.textSecondary; font.family: "SF Pro Text" }
                    }
                }
                
                Divider {}
                
                // Thresholds
                SettingRow {
                    title: "Yellow threshold (drop rate)"
                    description: "Trigger a warning alert when drop rate is above this threshold."
                    subtext: "Range: 0.0% — 5.0%"
                    RowLayout {
                        spacing: 16
                        Slider { 
                            id: yellowSlider
                            Layout.fillWidth: true; value: 0.1; from: 0.0; to: 5.0
                            background: Rectangle {
                                x: yellowSlider.leftPadding
                                y: yellowSlider.topPadding + yellowSlider.availableHeight / 2 - height / 2
                                width: yellowSlider.availableWidth
                                height: 4; radius: 2; color: Theme.bgTertiary
                                Rectangle { 
                                    width: yellowSlider.visualPosition * parent.width
                                    height: parent.height; color: Theme.statusAmber; radius: 2 
                                }
                            }
                            handle: Rectangle {
                                x: yellowSlider.leftPadding + yellowSlider.visualPosition * (yellowSlider.availableWidth - width)
                                y: yellowSlider.topPadding + yellowSlider.availableHeight / 2 - height / 2
                                width: 20; height: 20; radius: 10; color: "white"; border.color: Theme.bgTertiary; border.width: 1
                            }
                        }
                        Rectangle {
                            width: 72; height: 36; radius: 8; border.color: Theme.bgTertiary; border.width: 1; color: "white"
                            Text { anchors.centerIn: parent; text: "0.1"; font.family: "SF Pro Text"; font.pixelSize: 13; font.weight: Font.Medium; color: Theme.textPrimary }
                        }
                        Text { text: "%"; color: Theme.textSecondary; font.pixelSize: 13; font.family: "SF Pro Text" }
                    }
                }
                
                Divider {}
                
                SettingRow {
                    title: "Red threshold (drop rate)"
                    description: "Trigger a critical alert when drop rate is above this threshold."
                    subtext: "Range: 0.0% — 10.0%"
                    RowLayout {
                        spacing: 16
                        Slider { 
                            id: redSlider
                            Layout.fillWidth: true; value: 1.0; from: 0.0; to: 10.0
                            background: Rectangle {
                                x: redSlider.leftPadding
                                y: redSlider.topPadding + redSlider.availableHeight / 2 - height / 2
                                width: redSlider.availableWidth
                                height: 4; radius: 2; color: Theme.bgTertiary
                                Rectangle { 
                                    width: redSlider.visualPosition * parent.width
                                    height: parent.height; color: Theme.statusRed; radius: 2 
                                }
                            }
                            handle: Rectangle {
                                x: redSlider.leftPadding + redSlider.visualPosition * (redSlider.availableWidth - width)
                                y: redSlider.topPadding + redSlider.availableHeight / 2 - height / 2
                                width: 20; height: 20; radius: 10; color: "white"; border.color: Theme.bgTertiary; border.width: 1
                            }
                        }
                        Rectangle {
                            width: 72; height: 36; radius: 8; border.color: Theme.bgTertiary; border.width: 1; color: "white"
                            Text { anchors.centerIn: parent; text: "1.0"; font.family: "SF Pro Text"; font.pixelSize: 13; font.weight: Font.Medium; color: Theme.textPrimary }
                        }
                        Text { text: "%"; color: Theme.textSecondary; font.pixelSize: 13; font.family: "SF Pro Text" }
                    }
                }

                Divider {}
                
                // Notifications Switch
                SettingRow {
                    title: "Desktop notifications"
                    description: "Show system notifications for alert events."
                    Switch { checked: true; Layout.alignment: Qt.AlignRight }
                }

                Divider {}
                
                SettingRow {
                    title: "Sound alerts"
                    description: "Play a sound when an alert is triggered."
                    Switch { checked: true; Layout.alignment: Qt.AlignRight }
                }

                Divider {}

                SettingRow {
                    title: "Test notification"
                    description: "Send a test alert to verify your notification settings."
                    Button {
                        text: "Send test notification"
                        Layout.alignment: Qt.AlignRight
                        contentItem: Text {
                            text: parent.text
                            font: parent.font
                            color: Theme.navyPrimary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 8
                            border.color: Theme.bgTertiary
                            color: parent.down ? Theme.bgSecondary : "white"
                        }
                    }
                }
            }
        }
    }
    
    // Internal Helper Components
    component SettingRow : ColumnLayout {
        property string title: ""
        property string description: ""
        property string subtext: ""
        Layout.fillWidth: true
        Layout.margins: 16
        spacing: 8
        Text { text: title; font.weight: Font.Bold; font.pixelSize: 14; color: Theme.textPrimary }
        Text { text: description; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true }
        Text { 
            text: subtext
            font.pixelSize: 11
            color: Theme.textTertiary
            visible: text !== ""
            Layout.fillWidth: true
        }
        default property alias content: innerContent.data
        Item { id: innerContent; Layout.fillWidth: true; implicitHeight: childrenRect.height; Layout.topMargin: 4 }
    }
    
    component Divider : Rectangle {
        Layout.fillWidth: true; height: 1; color: Theme.bgTertiary
    }
}
