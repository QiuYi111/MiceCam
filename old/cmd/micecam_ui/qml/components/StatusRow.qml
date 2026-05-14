import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

RowLayout {
    id: root

    property color dotColor: Theme.success
    property string label: ""
    property string value: ""

    spacing: 10

    Rectangle {
        width: 8
        height: 8
        radius: 4
        color: root.dotColor
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        text: root.label
        color: Theme.textPrimary
        font.pixelSize: 12
        Layout.fillWidth: true
        elide: Text.ElideRight
    }

    Text {
        visible: root.value.length > 0
        text: root.value
        color: Theme.textSecondary
        font.pixelSize: 12
        font.weight: Font.Medium
        horizontalAlignment: Text.AlignRight
    }
}
