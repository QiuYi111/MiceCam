#include "RecordingSetup.h"

#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>

namespace {

bool containsSupportedFps(const QList<int>& supportedFps, double fps) {
    const int roundedFps = static_cast<int>(fps);
    for (const int supported : supportedFps) {
        if (supported == roundedFps) {
            return true;
        }
    }
    return false;
}

}  // namespace

QString sanitizeSessionName(const QString& sessionName) {
    QString sanitized = sessionName.trimmed();
    sanitized.replace(QRegularExpression(R"([<>:"/\\|?*\x00-\x1F]+)"), "_");
    sanitized.replace(QRegularExpression(R"(\s+)"), "_");
    sanitized.replace(QRegularExpression(R"(_{2,})"), "_");
    sanitized.remove(QRegularExpression(R"(^[._\s]+|[._\s]+$)"));
    return sanitized;
}

RecordingPreflightResult validateRecordingSetup(
    const QList<CaptureDeviceDescriptor>& devices,
    int selectedDeviceIndex,
    const QString& sessionName,
    const QString& outputDir,
    const QString& resolution,
    double fps,
    bool runtimeBusy
) {
    RecordingPreflightResult result;
    result.sanitizedSessionName = sanitizeSessionName(sessionName);
    result.resolvedOutputDir = QDir::cleanPath(outputDir.trimmed());

    if (runtimeBusy) {
        result.message = "Capture is busy. Wait for the current recording or export to finish.";
        return result;
    }

    if (devices.isEmpty()) {
        result.message = "Connect a camera to begin.";
        return result;
    }

    if (selectedDeviceIndex < 0 || selectedDeviceIndex >= devices.size()) {
        result.message = "Select an available camera.";
        return result;
    }

    const CaptureDeviceDescriptor& device = devices.at(selectedDeviceIndex);
    if (!device.available) {
        result.message = "The selected camera is no longer available.";
        return result;
    }

    if (result.sanitizedSessionName.isEmpty()) {
        result.message = "Enter a session name using filesystem-safe characters.";
        return result;
    }

    if (resolution.isEmpty() || !device.supportedResolutions.contains(resolution)) {
        result.message = "Choose a supported resolution for the selected camera.";
        return result;
    }

    if (fps <= 0.0 || !containsSupportedFps(device.supportedFps, fps)) {
        result.message = "Choose a supported FPS for the selected camera.";
        return result;
    }

    if (result.resolvedOutputDir.isEmpty()) {
        result.message = "Choose an output folder.";
        return result;
    }

    QDir rootDir;
    if (!rootDir.mkpath(result.resolvedOutputDir)) {
        result.message = "The output folder could not be created.";
        return result;
    }

    const QFileInfo outputInfo(result.resolvedOutputDir);
    if (!outputInfo.exists() || !outputInfo.isDir()) {
        result.message = "The output folder is not available.";
        return result;
    }

    QTemporaryFile probe(QDir(result.resolvedOutputDir).filePath(".micecam-write-test-XXXXXX"));
    probe.setAutoRemove(true);
    if (!probe.open()) {
        result.message = "The output folder is not writable.";
        return result;
    }

    result.ok = true;
    result.message = "Ready to record.";
    return result;
}
