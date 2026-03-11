import QtQuick
import QtQuick.Layouts
import "../theme/Theme.js" as Theme

Rectangle {
    id: root

    property bool compact: false
    property bool readyToRecord: false
    property bool busy: false
    property string sessionState: "idle"
    property string outputSummary: ""
    property bool autoDecode: true
    property string primaryText: "Start Recording"
    property string secondaryText: ""

    signal primaryClicked()
    signal secondaryClicked()

    radius: 12
    color: Theme.surface
    border.color: Theme.separator
    border.width: 1
    implicitHeight: compact ? 96 : 58

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space16

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.space6

                Text {
                    text: root.outputSummary
                    color: Theme.textPrimary
                    font.pixelSize: 12
                    font.weight: Font.Medium
                    wrapMode: Text.NoWrap
                    elide: Text.ElideMiddle
                }

                Text {
                    text: root.autoDecode ? "Auto-decode on" : "Auto-decode off"
                    color: Theme.textSecondary
                    font.pixelSize: 10
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10

                SecondaryButton {
                    visible: root.secondaryText.length > 0
                    text: root.secondaryText
                    implicitWidth: 126
                    onClicked: root.secondaryClicked()
                }

                PrimaryButton {
                    text: root.primaryText
                    implicitWidth: 146
                    enabled: root.sessionState === "recording" || (!root.busy && root.readyToRecord)
                    fillColor: root.sessionState === "recording" ? Theme.error : Theme.accent
                    onClicked: root.primaryClicked()
                }
            }
        }
    }
}
