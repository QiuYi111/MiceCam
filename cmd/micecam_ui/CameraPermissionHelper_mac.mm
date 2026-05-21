#include "CameraPermissionHelper.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

bool micecam_request_camera_access() {
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (status == AVAuthorizationStatusAuthorized) {
        return true;
    }
    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        return false;
    }
    __block bool granted = false;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL ok) {
        granted = ok;
        dispatch_semaphore_signal(sem);
    }];
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
    return granted;
}

std::vector<MacCameraDevice> micecam_enumerate_cameras() {
    std::vector<MacCameraDevice> result;
    @autoreleasepool {
        AVCaptureDeviceDiscoverySession* session =
            [AVCaptureDeviceDiscoverySession
                discoverySessionWithDeviceTypes:@[AVCaptureDeviceTypeExternal, AVCaptureDeviceTypeBuiltInWideAngleCamera]
                mediaType:AVMediaTypeVideo
                position:AVCaptureDevicePositionUnspecified];
        for (AVCaptureDevice* device in session.devices) {
            MacCameraDevice d;
            d.id = device.uniqueID.UTF8String;
            d.name = device.localizedName.UTF8String;
            result.push_back(d);
        }
    }
    return result;
}

#endif
#endif
