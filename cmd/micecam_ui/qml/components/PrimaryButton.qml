import QtQuick
import QtQuick.Controls
import "../theme/Theme.js" as Theme

Button {
    id: root

    property color fillColor: Theme.accent
    property color disabledFillColor: Theme.surfaceMuted
    property color disabledTextColor: Theme.textTertiary

    implicitHeight: 40
    font.pixelSize: 14
    font.weight: Font.Medium

    background: Rectangle {
        radius: Theme.radiusControl
        color: root.enabled
            ? (root.down ? Theme.accentPressed : root.fillColor)
            : root.disabledFillColor
    }

    contentItem: Text {
        text: root.text
        color: root.enabled ? "white" : root.disabledTextColor
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
