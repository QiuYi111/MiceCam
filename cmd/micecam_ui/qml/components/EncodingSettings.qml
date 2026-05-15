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
                font.family: Theme.fontPrimary
                font.pixelSize: 14
                color: Theme.navyPrimary
                MouseArea { anchors.fill: parent; onClicked: root.parent.parent.currentViewIndex = 0 }
            }
        }
        
        RowLayout {
            Text { text: "Encoding"; font.family: Theme.fontPrimary; font.pixelSize: 28; font.weight: Font.Bold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            Row {
                spacing: 8
                Text { text: "✓"; color: Theme.statusGreen; font.pixelSize: 16 }
                Text { text: "Saved automatically"; color: Theme.textSecondary; font.family: Theme.fontPrimary; font.pixelSize: 12 }
            }
        }
        
        // Settings Card
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: settingsCol.implicitHeight + 32
            color: "white"
            radius: 12
            border.color: Theme.bgTertiary
            
            ColumnLayout {
                id: settingsCol
                anchors.fill: parent
                anchors.margins: 16
                spacing: 0
                
                // Bitrate
                SettingRow {
                    title: "Default bitrate"
                    description: "Set the default target bitrate for all cameras."
                    RowLayout {
                        spacing: 16
                        Slider { 
                            id: bitrateSlider
                            Layout.fillWidth: true; value: appController.settings.defaultBitrateKbps / 1000; from: 3; to: 10
                            background: Rectangle {
                                x: bitrateSlider.leftPadding
                                y: bitrateSlider.topPadding + bitrateSlider.availableHeight / 2 - height / 2
                                width: bitrateSlider.availableWidth
                                height: 4; radius: 2; color: Theme.bgTertiary
                                Rectangle { 
                                    width: bitrateSlider.visualPosition * parent.width
                                    height: parent.height; color: Theme.navyPrimary; radius: 2 
                                }
                            }
                            handle: Rectangle {
                                x: bitrateSlider.leftPadding + bitrateSlider.visualPosition * (bitrateSlider.availableWidth - width)
                                y: bitrateSlider.topPadding + bitrateSlider.availableHeight / 2 - height / 2
                                width: 20; height: 20; radius: 10; color: "white"; border.color: Theme.bgTertiary; border.width: 1
                            }
                        }
                        Rectangle {
                            width: 80; height: 36; radius: 8; border.color: Theme.bgTertiary; border.width: 1; color: "white"
                            Text { anchors.centerIn: parent; text: Math.round(appController.settings.defaultBitrateKbps / 1000) + " Mbps"; font.family: Theme.fontPrimary; font.pixelSize: 13; font.weight: Font.Medium; color: Theme.textPrimary }
                        }
                    }
                }
                
                Divider {}
                
                // Keyframe
                SettingRow {
                    title: "Keyframe interval"
                    description: "Set how often a keyframe (I-frame) is inserted."
                    RowLayout {
                        spacing: 12
                        Rectangle {
                            width: 144; height: 36; radius: 8; border.color: Theme.bgTertiary; border.width: 1; clip: true
                            Row {
                                anchors.fill: parent; spacing: 0
                                Rectangle { width: 36; height: 36; color: "#F9FAFB"; Text { anchors.centerIn: parent; text: "—"; color: Theme.textPrimary } }
                                Rectangle { width: 1; height: 36; color: Theme.bgTertiary }
                                Rectangle { width: 70; height: 36; color: "white"; Text { anchors.centerIn: parent; text: "120"; font.weight: Font.Bold; color: Theme.textPrimary } }
                                Rectangle { width: 1; height: 36; color: Theme.bgTertiary }
                                Rectangle { width: 36; height: 36; color: "#F9FAFB"; Text { anchors.centerIn: parent; text: "+"; color: Theme.textPrimary } }
                            }
                        }
                        Text { text: "frames"; font.pixelSize: 13; color: Theme.textSecondary; font.family: Theme.fontPrimary }
                    }
                }
                
                Divider {}
                
                // Preset
                SettingRow {
                    title: "Encoder preset (preview)"
                    description: "Preview of quality vs. performance trade-offs."
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        Rectangle {
                            width: 400; height: 36; radius: 8; border.color: Theme.bgTertiary; border.width: 1; color: "white"
                            Row {
                                anchors.fill: parent; spacing: 0
                                Repeater {
                                    model: ["Ultrafast", "Fast", "Medium", "Slow", "Slower"]
                                    Rectangle {
                                        width: 80; height: 36
                                        color: modelData === "Medium" ? Theme.navyPrimary : "white"
                                        radius: (index === 0 || index === 4) ? 8 : 0 // ADDED ROUNDED CORNERS TO ENDS
                                        Text { 
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: modelData === "Medium" ? "white" : Theme.textPrimary
                                            font.pixelSize: 11; font.family: Theme.fontPrimary
                                        }
                                        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.bgTertiary; visible: index < 4 }
                                    }
                                }
                            }
                        }
                    }
                }

                Divider {}

                // Hardware Acceleration
                SettingRow {
                    title: "Hardware acceleration"
                    description: "Use GPU acceleration when available."
                    Switch { checked: appController.settings.hardwareAcceleration; Layout.alignment: Qt.AlignRight }
                }

                Divider {}

                // Preview Quality
                SettingRow {
                    title: "Preview quality"
                    description: "Affects only live preview performance."
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        Rectangle {
                            width: 300; height: 36; radius: 8; border.color: Theme.bgTertiary; border.width: 1; color: "white"
                            Row {
                                anchors.fill: parent; spacing: 0
                                Repeater {
                                    model: ["Low", "Medium", "High"]
                                    Rectangle {
                                        width: 100; height: 36
                                        color: modelData === "Medium" ? Theme.navyPrimary : "white"
                                        radius: (index === 0 || index === 2) ? 8 : 0 // ADDED ROUNDED CORNERS TO ENDS
                                        Text { 
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: modelData === "Medium" ? "white" : Theme.textPrimary
                                            font.pixelSize: 12; font.family: Theme.fontPrimary
                                        }
                                        Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.bgTertiary; visible: index < 2 }
                                    }
                                }
                            }
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
