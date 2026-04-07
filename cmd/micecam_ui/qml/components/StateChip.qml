import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

Rectangle {
    id: root

    property string text: ""
    property string state: "idle"

    function backgroundForState() {
        if (root.state === "recording") {
            return Theme.errorSoft
        }
        if (root.state === "decoding") {
            return Theme.warningSoft
        }
        if (root.state === "completed") {
            return Theme.successSoft
        }
        if (root.state === "error") {
            return Theme.errorSoft
        }
        return Theme.surfaceMuted
    }

    function foregroundForState() {
        if (root.state === "recording") {
            return Theme.error
        }
        if (root.state === "decoding") {
            return Theme.warning
        }
        if (root.state === "completed") {
            return Theme.success
        }
        if (root.state === "error") {
            return Theme.error
        }
        return Theme.textSecondary
    }

    radius: Theme.radiusPill
    color: backgroundForState()
    implicitHeight: 32
    implicitWidth: label.implicitWidth + Theme.space16 * 2

    RowLayout {
        anchors.centerIn: parent
        spacing: Theme.space8

        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: root.foregroundForState()
        }

        Text {
            id: label
            text: root.text
            color: root.foregroundForState()
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }
}
