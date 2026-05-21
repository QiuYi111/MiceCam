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
            
            LinkButton { text: "Documentation"; icon: "logging"; onClicked: Qt.openUrlExternally("https://github.com/QiuYi111/MiceCam/wiki") }
            LinkButton { text: "GitHub Repository"; icon: "encoding"; onClicked: Qt.openUrlExternally("https://github.com/QiuYi111/MiceCam") }
            LinkButton { text: "Check for Updates"; icon: "check"; onClicked: Qt.openUrlExternally("https://github.com/QiuYi111/MiceCam/releases") }
        }
    }
    
    component LinkButton : Item {
        property string text: ""
        property string icon: ""
        signal clicked()

        implicitWidth: rowLayout.implicitWidth
        implicitHeight: rowLayout.implicitHeight

        RowLayout {
            id: rowLayout
            spacing: 8
            
            AppIcon { name: parent.icon; size: 16; color: Theme.navyPrimary }
            
            Text {
                text: parent.parent.text
                font.family: Theme.fontPrimary
                font.pixelSize: 14
                color: Theme.navyPrimary
            }
        }
        
        MouseArea { 
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            hoverEnabled: true
            onEntered: parent.opacity = 0.7
            onExited: parent.opacity = 1.0
            onClicked: parent.clicked()
        }
    }
}
