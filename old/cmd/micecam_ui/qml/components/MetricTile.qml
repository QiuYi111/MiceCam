import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

Rectangle {
    id: root

    property string label: ""
    property string value: ""
    property color accentColor: Theme.textPrimary

    radius: 12
    color: Theme.surface
    border.color: Theme.separator
    border.width: 1

    implicitHeight: 72

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4

        Text {
            text: root.value
            color: root.accentColor
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }

        Text {
            text: root.label
            color: Theme.textSecondary
            font.pixelSize: 12
        }
    }
}
