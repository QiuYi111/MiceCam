import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property string pluginPath: ""
    property var detailData: ({})

    signal backClicked()

    function loadDetail() {
        if (pluginPath.length > 0) {
            detailData = appController.getPluginDetail(pluginPath)
        }
    }

    onPluginPathChanged: loadDetail()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 24

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Button {
                background: Rectangle {
                    radius: 6
                    color: Theme.bgSecondary
                    implicitWidth: 80
                    implicitHeight: 32
                }
                contentItem: Text {
                    text: "\u2190 Back"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    color: Theme.navyPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: root.backClicked()
            }

            Text {
                text: detailData.name || "Plugin Detail"
                font.family: Theme.fontPrimary
                font.pixelSize: 24
                font.weight: Font.Bold
                color: Theme.textPrimary
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                visible: detailData.status !== undefined
                width: statusText.implicitWidth + 16
                height: 24
                radius: 12
                color: detailData.status === "OK" ? "#E8F5E9"
                     : detailData.status === "Error" ? "#FFEBEE"
                     : Theme.bgTertiary

                Text {
                    id: statusText
                    anchors.centerIn: parent
                    text: detailData.status || ""
                    font.family: Theme.fontPrimary
                    font.pixelSize: 11
                    font.weight: Font.Medium
                    color: detailData.status === "OK" ? "#2E7D32"
                         : detailData.status === "Error" ? Theme.statusRed
                         : Theme.textSecondary
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(implicitWidth, 600)
                spacing: 20

                GridLayout {
                    columns: 2
                    columnSpacing: 24
                    rowSpacing: 12
                    Layout.fillWidth: true

                    Text {
                        text: "Plugin ID"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.pluginId || "-"
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Version"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.pluginVersion || "-"
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "API Version"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.apiVersion !== undefined ? String(detailData.apiVersion) : "-"
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Type"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.type || "-"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Enabled"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.enabled !== undefined ? (detailData.enabled ? "Yes" : "No") : "-"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        color: detailData.enabled ? Theme.statusGreen : Theme.statusRed
                    }

                    Text {
                        text: "Process Model"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.processModel || "-"
                        font.family: Theme.fontMono
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Author"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.author || "-"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        color: Theme.textPrimary
                    }

                    Text {
                        text: "Path"
                        font.family: Theme.fontPrimary
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        color: Theme.textSecondary
                    }
                    Text {
                        text: detailData.path || "-"
                        font.family: Theme.fontMono
                        font.pixelSize: 11
                        color: Theme.textSecondary
                        wrapMode: Text.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.divider
                }

                Text {
                    text: "Description"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }
                Text {
                    text: detailData.description || "No description available."
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Text {
                    text: "Required Features"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }
                Text {
                    text: {
                        var feats = detailData.requiredFeatures
                        return feats && feats.length > 0 ? feats.join(", ") : "None"
                    }
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Text {
                    text: "Optional Features"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }
                Text {
                    text: {
                        var feats = detailData.optionalFeatures
                        return feats && feats.length > 0 ? feats.join(", ") : "None"
                    }
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Text {
                    text: "Platforms"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                }
                Text {
                    text: {
                        var plats = detailData.platforms
                        if (!plats) return "-"
                        var keys = Object.keys(plats)
                        return keys.length > 0
                            ? keys.map(function(k) { return k + ": " + plats[k] }).join("\n")
                            : "-"
                    }
                    font.family: Theme.fontMono
                    font.pixelSize: 12
                    color: Theme.textPrimary
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.divider
                    visible: diagList.text.length > 0
                }

                Text {
                    text: "Diagnostics"
                    font.family: Theme.fontPrimary
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: Theme.textSecondary
                    visible: diagList.text.length > 0
                }
                Text {
                    id: diagList
                    text: {
                        var diags = detailData.diagnostics
                        return diags && diags.length > 0 ? diags.join("\n") : ""
                    }
                    font.family: Theme.fontMono
                    font.pixelSize: 11
                    color: Theme.statusRed
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    visible: text.length > 0
                }
            }
        }
    }
}
