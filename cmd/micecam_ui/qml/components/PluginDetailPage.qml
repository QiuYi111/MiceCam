import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string pluginPath: ""
    property var detailData: ({})
    readonly property bool hasDiagnostics: detailData.diagnostics !== undefined && detailData.diagnostics.length > 0

    signal backClicked()

    function loadDetail() {
        if (pluginPath.length > 0) {
            detailData = appController.getPluginDetail(pluginPath)
        }
    }

    function featureText(values) {
        return values && values.length > 0 ? values.join(", ") : "None"
    }

    function platformText(values) {
        if (!values) return "-"
        var keys = Object.keys(values)
        return keys.length > 0 ? keys.map(function(k) { return k + ": " + values[k] }).join("\n") : "-"
    }

    onPluginPathChanged: loadDetail()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 18

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Button {
                text: "\u2039  Back to Plugins"
                background: Rectangle {
                    radius: 6
                    color: Theme.bgSecondary
                    border.color: Theme.borderColor
                    implicitWidth: 132
                    implicitHeight: 34
                }
                contentItem: Text {
                    text: parent.text
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    color: Theme.navyPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.backClicked()
            }

            AppIcon {
                name: detailData.type === "bundled" ? "camera" : "gear"
                size: 28
                color: Theme.textPrimary
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                RowLayout {
                    spacing: 10
                    Text {
                        text: detailData.name || "Plugin Detail"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 26
                        font.weight: Font.Bold
                        color: Theme.textPrimary
                    }
                    Badge {
                        text: detailData.type === "bundled" ? "Bundled" : "Linked"
                        tone: "blue"
                    }
                    Badge {
                        text: detailData.apiVersion !== undefined ? "API v" + detailData.apiVersion : "API"
                        tone: "gray"
                    }
                    Badge {
                        text: detailData.restartRequired ? "Restart required" : (detailData.status || "")
                        tone: detailData.restartRequired ? "amber" : (detailData.status === "OK" ? "green" : "red")
                        visible: text.length > 0
                    }
                }
                Text {
                    text: detailData.description || "Configure plugin behavior and inspect runtime status."
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    color: Theme.textSecondary
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Button {
                text: "Plugin Documentation"
                background: Rectangle {
                    radius: 6
                    color: Theme.bgPrimary
                    border.color: Theme.borderColor
                    implicitHeight: 34
                }
                contentItem: Text {
                    text: parent.text
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    color: Theme.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(parent.width - 24, 760)
                spacing: 14

                SectionPanel {
                    title: "Diagnostics"
                    visible: root.hasDiagnostics
                    Layout.fillWidth: true

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: parent.contentTop
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 0

                        Repeater {
                            model: detailData.diagnostics || []
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 72
                                color: "transparent"
                                border.color: Theme.divider

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 12
                                    AppIcon { name: "warning"; size: 18; color: Theme.statusAmber }
                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 3
                                        Text {
                                            text: "Plugin diagnostic"
                                            font.family: Theme.fontPrimary
                                            font.pixelSize: 14
                                            font.weight: Font.DemiBold
                                            color: Theme.textPrimary
                                        }
                                        Text {
                                            text: modelData
                                            font.family: Theme.fontPrimary
                                            font.pixelSize: 12
                                            color: Theme.textSecondary
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                    Badge { text: "Recoverable"; tone: "amber" }
                                }
                            }
                        }
                    }
                }

                SectionPanel {
                    title: "Manifest"
                    Layout.fillWidth: true
                    implicitHeight: 356

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: parent.contentTop
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 0

                        DetailRow { label: "Plugin ID"; value: detailData.pluginId || "-" }
                        DetailRow { label: "Display name"; value: detailData.name || "-" }
                        DetailRow { label: "Version"; value: detailData.pluginVersion || "-" }
                        DetailRow { label: "API version"; value: detailData.apiVersion !== undefined ? "v" + detailData.apiVersion : "-" }
                        DetailRow { label: "Source type"; value: detailData.type || "-" }
                        DetailRow { label: "Process model"; value: detailData.processModel || "-" }
                        DetailRow { label: "Plugin path"; value: detailData.path || "-" }
                        DetailRow { label: "Platform entrypoints"; value: root.platformText(detailData.platforms) }
                    }
                }

                SectionPanel {
                    title: "Capabilities"
                    Layout.fillWidth: true
                    implicitHeight: 118

                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: parent.contentTop
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 12

                        CapabilityChip { text: root.featureText(detailData.requiredFeatures) === "None" ? "No required features" : root.featureText(detailData.requiredFeatures) }
                        CapabilityChip { text: root.featureText(detailData.optionalFeatures) === "None" ? "No optional features" : root.featureText(detailData.optionalFeatures) }
                        CapabilityChip { text: "Schema-ready config" }
                        CapabilityChip { text: "External process" }
                    }
                }

                SectionPanel {
                    title: "Devices"
                    Layout.fillWidth: true
                    implicitHeight: 118

                    RowLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: parent.contentTop
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 12

                        Rectangle {
                            width: 10; height: 10; radius: 5
                            color: Theme.textTertiary
                        }
                        Text {
                            text: "0 devices"
                            font.family: Theme.fontPrimary
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: Theme.textPrimary
                        }
                        Text {
                            text: "No devices are currently reported by this plugin."
                            font.family: Theme.fontPrimary
                            font.pixelSize: 13
                            color: Theme.textSecondary
                            Layout.fillWidth: true
                        }
                    }
                }

                SectionPanel {
                    title: "Configuration"
                    Layout.fillWidth: true
                    implicitHeight: 168

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.topMargin: parent.contentTop
                        anchors.leftMargin: 18
                        anchors.rightMargin: 18
                        spacing: 10

                        ConfigRow { label: "Width"; value: "Default"; mode: "pre-open" }
                        ConfigRow { label: "Height"; value: "Default"; mode: "pre-open" }
                        ConfigRow { label: "Framerate"; value: "Default"; mode: "pre-open" }
                    }
                }
            }
        }
    }

    component SectionPanel : Rectangle {
        id: panel
        property string title: ""
        readonly property int contentTop: 56
        radius: 8
        color: Theme.bgPrimary
        border.color: Theme.borderColor
        implicitHeight: 96

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 18
            anchors.topMargin: 16
            text: panel.title
            font.family: Theme.fontPrimary
            font.pixelSize: 16
            font.weight: Font.DemiBold
            color: Theme.textPrimary
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 48
            height: 1
            color: Theme.divider
        }
    }

    component DetailRow : Rectangle {
        property string label: ""
        property string value: ""
        Layout.fillWidth: true
        height: Math.max(32, valueText.implicitHeight + 12)
        color: "transparent"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.divider
        }

        RowLayout {
            anchors.fill: parent
            spacing: 16
            Text {
                text: label
                font.family: Theme.fontPrimary
                font.pixelSize: 12
                font.weight: Font.Medium
                color: Theme.textSecondary
                Layout.preferredWidth: 210
            }
            Text {
                id: valueText
                text: value
                font.family: value.indexOf("/") >= 0 || value.indexOf(".") >= 0 ? Theme.fontMono : Theme.fontPrimary
                font.pixelSize: 12
                color: Theme.textPrimary
                wrapMode: Text.WrapAnywhere
                Layout.fillWidth: true
            }
        }
    }

    component Badge : Rectangle {
        property string text: ""
        property string tone: "gray"
        width: badgeText.implicitWidth + 18
        height: 26
        radius: 8
        color: tone === "green" ? "#E8F5E9"
             : tone === "red" ? "#FFEBEE"
             : tone === "amber" ? "#FFF7ED"
             : tone === "blue" ? Theme.navyTint
             : Theme.bgSecondary
        Text {
            id: badgeText
            anchors.centerIn: parent
            text: parent.text
            font.family: Theme.fontPrimary
            font.pixelSize: 12
            font.weight: Font.Medium
            color: parent.tone === "green" ? "#2E7D32"
                 : parent.tone === "red" ? Theme.statusRed
                 : parent.tone === "amber" ? Theme.statusAmber
                 : parent.tone === "blue" ? Theme.navyPrimary
                 : Theme.textSecondary
        }
    }

    component CapabilityChip : Rectangle {
        property string text: ""
        height: 36
        width: chipText.implicitWidth + 28
        radius: 8
        color: Theme.bgSecondary
        border.color: Theme.borderColor
        Text {
            id: chipText
            anchors.centerIn: parent
            text: parent.text
            font.family: Theme.fontPrimary
            font.pixelSize: 12
            color: Theme.textPrimary
        }
    }

    component ConfigRow : Rectangle {
        property string label: ""
        property string value: ""
        property string mode: ""
        Layout.fillWidth: true
        height: 32
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            Text {
                text: label
                font.family: Theme.fontPrimary
                font.pixelSize: 13
                color: Theme.textPrimary
                Layout.preferredWidth: 180
            }
            Rectangle {
                Layout.preferredWidth: 160
                height: 28
                radius: 6
                color: Theme.bgSecondary
                border.color: Theme.borderColor
                Text {
                    anchors.centerIn: parent
                    text: value
                    font.family: Theme.fontPrimary
                    font.pixelSize: 12
                    color: Theme.textSecondary
                }
            }
            Badge { text: mode; tone: "blue" }
            Item { Layout.fillWidth: true }
        }
    }
}
