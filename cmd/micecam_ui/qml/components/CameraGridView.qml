import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MiceCam.Models
import "../theme"

Item {
    id: root
    
    ScrollView {
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
            anchors.margins: 16
            spacing: 16
            
            RowLayout {
                spacing: 16
                Layout.fillHeight: true
                Layout.preferredHeight: root.height / 2 - 24
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_A"; fps: 29.97; drops: 0; status: 0
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_B"; fps: 29.97; drops: 0; status: 0
                }
            }
            
            RowLayout {
                spacing: 16
                Layout.fillHeight: true
                Layout.preferredHeight: root.height / 2 - 24
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_C"; fps: 29.97; drops: 0; status: 0
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "CAM_D"; fps: 18.45; drops: 152; status: 1
                }
                CameraCard {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    cameraName: "USB-1"; fps: 29.97; drops: 0; status: 0
                }
            }
        }
    }
}
