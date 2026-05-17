import QtQuick
import QtQuick.Controls
import "../theme/Theme.js" as Theme

Button {
    id: root

    implicitHeight: 40
    font.pixelSize: 14
    font.weight: Font.Medium

    background: Rectangle {
        radius: Theme.radiusControl
        color: root.down ? Theme.surfaceMuted : Theme.surfaceRaised
        border.color: root.enabled ? Theme.borderSubtle : Theme.separator
        border.width: 1
    }

    contentItem: Text {
        text: root.text
        color: root.enabled ? Theme.textPrimary : Theme.textTertiary
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
