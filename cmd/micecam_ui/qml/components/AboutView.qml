import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root
    
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 32
        
        // Banner
        Image {
            source: "qrc:/MiceCam/UI/resources/banner.png"
            Layout.preferredWidth: 400
            Layout.preferredHeight: 120
            fillMode: Image.PreserveAspectFit
            Layout.alignment: Qt.AlignHCenter
        }
        
        Text {
            text: "Version " + appController.appVersion + " (Build " + appController.buildDate + ")"
            font.family: Theme.fontPrimary
            font.pixelSize: 18
            color: Theme.textSecondary
            Layout.alignment: Qt.AlignHCenter
        }
        
        Text {
            text: "High-performance scientific camera capture system.\nDesigned for reliable, continuous observation and recording."
            font.family: Theme.fontPrimary
            font.pixelSize: 14
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.4
            Layout.alignment: Qt.AlignHCenter
        }
        
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 32
            
            LinkButton { text: "Documentation"; icon: "logging" }
            LinkButton { text: "GitHub Repository"; icon: "encoding" }
            LinkButton { text: "Check for Updates"; icon: "check" }
        }
    }
    
    component LinkButton : RowLayout {
        property string text: ""
        property string icon: ""
        spacing: 8
        
        AppIcon { name: icon; size: 16; color: Theme.navyPrimary }
        
        Text {
            text: parent.text
            font.family: Theme.fontPrimary
            font.pixelSize: 14
            color: Theme.navyPrimary
        }
        
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            MouseArea { 
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.parent.opacity = 0.7
                onExited: parent.parent.opacity = 1.0
            }
        }
    }
}
