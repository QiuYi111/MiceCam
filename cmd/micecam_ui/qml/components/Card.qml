import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

Rectangle {
    id: root

    property string title: ""
    property string subtitle: ""
    property color fillColor: Theme.surfaceRaised

    default property alias content: body.data

    radius: Theme.radiusCard
    color: fillColor
    border.color: Theme.borderSubtle
    border.width: 1

    implicitHeight: contentColumn.implicitHeight + 32

    ColumnLayout {
        id: contentColumn
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        ColumnLayout {
            visible: root.title.length > 0 || root.subtitle.length > 0
            spacing: Theme.space6

            Text {
                visible: root.title.length > 0
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: 15
                font.weight: Font.Medium
            }

            Text {
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.textSecondary
                font.pixelSize: 12
                wrapMode: Text.Wrap
            }
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            spacing: Theme.space12
        }
    }
}
