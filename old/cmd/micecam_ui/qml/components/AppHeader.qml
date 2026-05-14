import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

Rectangle {
    id: root

    property string sessionState: "idle"
    property string statusHeadline: ""
    property string statusDetail: ""
    property string sessionName: ""
    property string outputDir: ""
    property bool compact: false

    radius: Theme.radiusCard
    color: Theme.surfaceRaised
    border.color: Theme.borderSubtle
    border.width: 1
    implicitHeight: compact ? 74 : 68

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: "MiceCam"
                    color: Theme.textPrimary
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                }

                Text {
                    text: root.sessionName.length > 0 ? root.sessionName : root.statusHeadline
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            StateChip {
                text: root.sessionState === "idle" ? "Idle" :
                      root.sessionState === "recording" ? "Recording" :
                      root.sessionState === "decoding" ? "Decoding" :
                      root.sessionState === "completed" ? "Completed" :
                      root.sessionState === "error" ? "Needs attention" : "Ready"
                state: root.sessionState
                Layout.alignment: Qt.AlignVCenter
            }
        }
    }
}
