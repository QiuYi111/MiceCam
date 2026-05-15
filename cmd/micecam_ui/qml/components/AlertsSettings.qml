import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Flickable {
    id: root
    contentHeight: mainCol.height
    clip: true

    ScrollBar.vertical: ScrollBar {}

    ColumnLayout {
        id: mainCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 32
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            Text {
                text: "\u2039 Cameras"
                font.family: Theme.fontPrimary
                font.pixelSize: 14
                color: Theme.navyPrimary
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.parent.parent.currentViewIndex = 0 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Text { text: "Alerts"; font.family: Theme.fontPrimary; font.pixelSize: 28; font.weight: Font.Bold; color: Theme.textPrimary }
            Item { Layout.fillWidth: true }
            Row {
                spacing: 6
                AppIcon {
                    name: "check"
                    size: 12
                    color: Theme.statusGreen
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text { text: "All changes saved automatically"; color: Theme.statusGreen; font.family: Theme.fontPrimary; font.pixelSize: 12; font.weight: Font.Medium }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: listCol.implicitHeight + 24
            color: "white"
            radius: 10
            border.color: Theme.borderColor
            border.width: 1

            ColumnLayout {
                id: listCol
                anchors.fill: parent
                anchors.topMargin: 12
                anchors.bottomMargin: 12
                spacing: 0

                SettingRow {
                    title: "Feishu webhook URL"
                    description: "Send alert notifications to a Feishu group via incoming webhook."
                    Layout.fillWidth: true
                    controlItem:                         TextField {
                        id: webhookField
                        width: 320
                        height: 32
                        text: appController.settings.webhookUrl
                        placeholderText: "Enter webhook URL"
                        echoMode: TextInput.Password
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        leftPadding: 10
                        rightPadding: 36
                        verticalAlignment: Text.AlignVCenter
                        background: Rectangle {
                            radius: 6
                            border.color: webhookField.activeFocus ? Theme.navyPrimary : Theme.borderColor
                            border.width: 1
                            color: "white"
                            AppIcon {
                                name: "eye"
                                size: 14
                                color: Theme.textTertiary
                                anchors.right: parent.right
                                anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                    extraText: "Your webhook URL is stored securely and never shared."
                }

                SettingDivider {}

                SettingRow {
                    title: "Watchdog timeout"
                    description: "Trigger an alert if a camera stops sending frames for the specified time."
                    Layout.fillWidth: true
                    controlItem: Row {
                        spacing: 8
                        StepperControl {
                            id: watchdogStepper
                            value: appController.settings.watchdogTimeout
                            minValue: 1
                            maxValue: 120
                        }
                        Text { text: "seconds"; font.family: Theme.fontPrimary; font.pixelSize: 13; color: Theme.textSecondary; anchors.verticalCenter: parent.verticalCenter }
                    }
                }

                SettingDivider {}

                SettingRow {
                    title: "Yellow threshold (drop rate)"
                    description: "Warning alert when drop rate exceeds this value."
                    Layout.fillWidth: true
                    controlItem: ThresholdControl {
                        id: yellowThreshold
                        initialValue: appController.settings.yellowDropThreshold
                        sliderFrom: 0.0
                        sliderTo: 5.0
                        sliderStep: 0.1
                        trackColor: Theme.statusAmber
                        rangeText: "Range: 0.0% \u2013 5.0%"
                    }
                }

                SettingDivider {}

                SettingRow {
                    title: "Red threshold (drop rate)"
                    description: "Critical alert when drop rate exceeds this value."
                    Layout.fillWidth: true
                    controlItem: ThresholdControl {
                        id: redThreshold
                        initialValue: appController.settings.redDropThreshold
                        sliderFrom: 0.0
                        sliderTo: 10.0
                        sliderStep: 0.1
                        trackColor: Theme.statusRed
                        rangeText: "Range: 0.0% \u2013 10.0%"
                    }
                }

                SettingDivider {}

                SettingRow {
                    title: "Desktop notifications"
                    description: "Show system notifications for alert events."
                    Layout.fillWidth: true
                    controlItem: HigSwitch { checked: true }
                }

                SettingDivider {}

                SettingRow {
                    title: "Sound alerts"
                    description: "Play a sound when an alert is triggered."
                    Layout.fillWidth: true
                    controlItem: HigSwitch { checked: true }
                }

                SettingDivider {}

                SettingRow {
                    title: "Test notification"
                    description: "Send a test alert to verify your settings."
                    Layout.fillWidth: true
                    controlItem: Button {
                        contentItem: Row {
                            spacing: 6
                            anchors.centerIn: parent
                            AppIcon {
                                name: "fullscreen"
                                size: 12
                                color: Theme.navyPrimary
                                anchors.verticalCenter: parent.verticalCenter
                                rotation: -45
                            }
                            Text {
                                text: "Send test notification"
                                font.family: Theme.fontPrimary
                                font.pixelSize: 13
                                color: Theme.navyPrimary
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                        background: Rectangle {
                            implicitWidth: 200
                            implicitHeight: 32
                            radius: 6
                            border.color: Theme.borderColor
                            border.width: 1
                            color: parent.down ? Theme.bgSecondary : "white"
                        }
                    }
                }
            }
        }
    }

    component StepperControl : Rectangle {
        id: stepperRoot
        width: 144
        height: 36
        radius: 8
        color: Theme.bgSecondary
        border.color: Theme.borderColor
        border.width: 1

        property int value: 3
        property int minValue: 1
        property int maxValue: 120

        Row {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                width: stepperRoot.height
                height: stepperRoot.height
                radius: 8
                color: minusArea.pressed ? Theme.bgTertiary : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "\u2212"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 18
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }
                MouseArea {
                    id: minusArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: stepperRoot.value = Math.max(stepperRoot.minValue, stepperRoot.value - 1)
                }
            }

            Rectangle {
                width: stepperRoot.width - 2 * stepperRoot.height
                height: stepperRoot.height
                color: "transparent"
                Text {
                    anchors.centerIn: parent
                    text: stepperRoot.value
                    font.family: Theme.fontPrimary
                    font.pixelSize: 15
                    font.weight: Font.Medium
                    color: Theme.textPrimary
                }
            }

            Rectangle {
                width: stepperRoot.height
                height: stepperRoot.height
                radius: 8
                color: plusArea.pressed ? Theme.bgTertiary : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "+"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 18
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }
                MouseArea {
                    id: plusArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: stepperRoot.value = Math.min(stepperRoot.maxValue, stepperRoot.value + 1)
                }
            }
        }
    }

    component HigSwitch : Rectangle {
        id: switchRoot
        width: 52
        height: 30
        radius: 15
        property bool checked: false
        color: checked ? Theme.navyPrimary : Theme.bgTertiary
        Behavior on color { ColorAnimation { duration: 150 } }

        Rectangle {
            id: knob
            width: 26
            height: 26
            radius: 13
            color: "white"
            border.color: "#C0C0C0"
            border.width: 0.5
            x: switchRoot.checked ? switchRoot.width - width - 2 : 2
            y: 2
            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.InOutQuad } }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: switchRoot.checked = !switchRoot.checked
        }
    }

    component ThresholdControl : Row {
        id: tcRoot
        spacing: 12
        height: 36

        property real initialValue: 0.1
        property real sliderFrom: 0.0
        property real sliderTo: 5.0
        property real sliderStep: 0.1
        property color trackColor: Theme.statusAmber
        property string rangeText: ""

        property real _ratio: (initialValue - sliderFrom) / (sliderTo - sliderFrom)
        property real _displayValue: initialValue

        Rectangle {
            id: trackBg
            width: 340
            height: 4
            radius: 2
            color: Theme.bgTertiary
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                width: thumb.x + thumb.width / 2
                height: parent.height
                radius: 2
                color: tcRoot.trackColor
            }

            Rectangle {
                id: thumb
                width: 18
                height: 18
                radius: 9
                color: "white"
                border.color: tcRoot.trackColor
                border.width: 2
                anchors.verticalCenter: parent.verticalCenter
                x: tcRoot._ratio * (trackBg.width - width)

                onXChanged: {
                    if (dragArea.drag.active) {
                        var r = x / (trackBg.width - width)
                        var val = tcRoot.sliderFrom + r * (tcRoot.sliderTo - tcRoot.sliderFrom)
                        val = Math.round(val / tcRoot.sliderStep) * tcRoot.sliderStep
                        tcRoot._displayValue = val
                    }
                }

                MouseArea {
                    id: dragArea
                    anchors.fill: parent
                    anchors.margins: -8
                    drag.target: thumb
                    drag.axis: Drag.XAxis
                    drag.minimumX: 0
                    drag.maximumX: trackBg.width - thumb.width
                    drag.smoothed: true
                    cursorShape: Qt.PointingHandCursor
                }
            }
        }

        Rectangle {
            width: 72
            height: 36
            radius: 6
            border.color: Theme.borderColor
            border.width: 1
            color: "white"
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: tcRoot._displayValue.toFixed(1)
                font.family: Theme.fontPrimary
                font.pixelSize: 13
                font.weight: Font.Medium
                color: Theme.textPrimary
            }
        }

        Text {
            text: "%"
            color: Theme.textSecondary
            font.family: Theme.fontPrimary
            font.pixelSize: 13
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: tcRoot.rangeText
            color: Theme.textTertiary
            font.family: Theme.fontPrimary
            font.pixelSize: 11
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    component SettingRow : RowLayout {
        id: settingRow
        property string title: ""
        property string description: ""
        property string extraText: ""
        property alias controlItem: controlSlot.data
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        spacing: 24

        ColumnLayout {
            Layout.preferredWidth: 300
            Layout.maximumWidth: 300
            Layout.minimumWidth: 300
            spacing: 2
            Text { text: settingRow.title; font.family: Theme.fontPrimary; font.weight: Font.Bold; font.pixelSize: 13; color: Theme.textPrimary; Layout.fillWidth: true }
            Text { text: settingRow.description; font.family: Theme.fontPrimary; font.pixelSize: 12; color: Theme.textSecondary; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            Text { text: settingRow.extraText; font.family: Theme.fontPrimary; font.pixelSize: 11; color: Theme.textTertiary; Layout.fillWidth: true; wrapMode: Text.WordWrap; visible: settingRow.extraText !== "" }
        }

        Item {
            id: controlSlot
            Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            Layout.preferredWidth: 560
            Layout.minimumWidth: 560
            implicitWidth: childrenRect.width
            implicitHeight: childrenRect.height
        }
    }

    component SettingDivider : Rectangle {
        Layout.fillWidth: true
        Layout.leftMargin: 16
        Layout.rightMargin: 16
        height: 1
        color: Theme.divider
    }
}
