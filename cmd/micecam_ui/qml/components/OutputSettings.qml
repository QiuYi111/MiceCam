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
            Text { text: "Output"; font.family: Theme.fontPrimary; font.pixelSize: 28; font.weight: Font.Bold; color: Theme.textPrimary }
        }
        
        // Settings Card
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
                
                // Output Directory
                SettingRow {
                    title: "Output Directory"
                    description: "Select where your recordings will be saved."
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        TextField { 
                            Layout.fillWidth: true
                            text: "/Volumes/Recordings/MiceCam" 
                            font.pixelSize: 13
                            background: Rectangle { border.color: Theme.bgTertiary; radius: 8; color: "white" }
                        }
                        Button {
                            text: "Browse..."
                            contentItem: Text { text: parent.text; font: parent.font; color: Theme.navyPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { radius: 8; border.color: Theme.bgTertiary; color: parent.down ? Theme.bgSecondary : "white" }
                        }
                    }
                }
                
                Divider {}
                
                // Directory Management
                SettingRow {
                    title: "Directory Management"
                    description: "Create subfolder for each session automatically."
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Create subfolder per session"; font.pixelSize: 13; Layout.fillWidth: true; color: Theme.textPrimary }
                        Switch { checked: true }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Folder Name Prefix"; font.pixelSize: 13; color: Theme.textPrimary }
                        TextField { 
                            text: "session_"
                            Layout.preferredWidth: 150
                            font.pixelSize: 13
                            background: Rectangle { border.color: Theme.bgTertiary; radius: 8; color: "white" }
                        }
                    }
                    Text { 
                        text: "Example: session_2026-05-13_14-30-00"
                        font.pixelSize: 11; color: Theme.textTertiary 
                    }
                }
                
                Divider {}
                
                // File Naming
                SettingRow {
                    title: "File Naming"
                    description: "Naming convention for camera output files."
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Naming Pattern"; font.pixelSize: 13; color: Theme.textPrimary }
                        ComboBox {
                            model: ["CAM_NAME_YYYYMMDD_HHMMSS", "SESSION_CAM_NAME", "INDEX"]
                            Layout.fillWidth: true
                            background: Rectangle { radius: 8; border.color: Theme.bgTertiary; color: "white" }
                        }
                    }
                }
                
                Divider {}
                
                // Container Format
                SettingRow {
                    title: "Container Format"
                    description: "The video container format for recordings."
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Format"; font.pixelSize: 13; color: Theme.textPrimary }
                        ComboBox {
                            model: ["MP4", "MKV", "MOV"]
                            Layout.fillWidth: true
                            background: Rectangle { radius: 8; border.color: Theme.bgTertiary; color: "white" }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Max File Size (GB)"; font.pixelSize: 13; color: Theme.textPrimary }
                        SpinBox {
                            from: 1; to: 100; value: 8
                            background: Rectangle { radius: 8; border.color: Theme.bgTertiary; color: "white" }
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
        Layout.fillWidth: true
        Layout.margins: 16
        spacing: 12
        Text { text: title; font.weight: Font.Bold; font.pixelSize: 14; color: Theme.textPrimary }
        Text { text: description; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true }
        default property alias content: innerContent.data
        ColumnLayout { id: innerContent; Layout.fillWidth: true; spacing: 12 }
    }
    
    component Divider : Rectangle {
        Layout.fillWidth: true; height: 1; color: Theme.bgTertiary
    }
}
