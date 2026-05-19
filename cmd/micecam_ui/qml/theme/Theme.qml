pragma Singleton

import QtQuick

QtObject {
    readonly property color navyPrimary: "#1B2A4A"
    readonly property color navyLight: "#2D3E5C"
    readonly property color navyDark: "#0F1A2E"
    readonly property color navyTint: "#E8ECF4"
    readonly property color navySelected: "#253756"

    readonly property color statusGreen: "#34C759"
    readonly property color statusAmber: "#FF9500"
    readonly property color statusRed: "#FF3B30"
    readonly property color recordRed: "#FF3B30"

    readonly property color bgPrimary: "#FFFFFF"
    readonly property color bgSecondary: "#F2F2F7"
    readonly property color bgTertiary: "#D1D1D6"

    readonly property color textPrimary: "#1C1C1E"
    readonly property color textSecondary: "#6E6E73"
    readonly property color textTertiary: "#AEAEB2"
    readonly property color overlayBg: "#99000000"

    readonly property color divider: "#C6C6C8"
    readonly property color borderColor: "#D1D1D6"

    readonly property string fontPrimary: Qt.platform.os === "osx" ? "Helvetica Neue" : Qt.platform.os === "windows" ? "Segoe UI" : "sans-serif"
    readonly property string fontMono: Qt.platform.os === "osx" ? "Menlo" : Qt.platform.os === "windows" ? "Consolas" : "monospace"
    readonly property string fontWeightMedium: "Medium"
}
