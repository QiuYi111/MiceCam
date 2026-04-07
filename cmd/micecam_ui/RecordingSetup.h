#pragma once

#include <QMetaType>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVariantList>

struct CaptureDeviceDescriptor {
    QString deviceId;
    QString backendId;
    QString displayName;
    QString backendDeviceName;
    int deviceIndex = -1;
    bool available = false;
    QStringList supportedResolutions;
    QList<int> supportedFps;
};

struct RecordingPreflightResult {
    bool ok = false;
    QString message;
    QString sanitizedSessionName;
    QString resolvedOutputDir;
};

QString sanitizeSessionName(const QString& sessionName);
RecordingPreflightResult validateRecordingSetup(
    const QList<CaptureDeviceDescriptor>& devices,
    int selectedDeviceIndex,
    const QString& sessionName,
    const QString& outputDir,
    const QString& resolution,
    double fps,
    bool runtimeBusy
);

Q_DECLARE_METATYPE(CaptureDeviceDescriptor)
