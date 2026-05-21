#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_OSX

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

static bool requestCameraPermission() {
    AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (status == AVAuthorizationStatusAuthorized) {
        return true;
    }
    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        return false;
    }
    // NotDetermined — trigger the permission dialog
    __block bool granted = false;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL ok) {
        granted = ok;
        dispatch_semaphore_signal(sem);
    }];
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
    return granted;
}

#endif // TARGET_OS_OSX
#endif // __APPLE__

extern "C" bool micecam_request_camera_access() {
#ifdef __APPLE__
#if TARGET_OS_OSX
    return requestCameraPermission();
#else
    return true;
#endif
#else
    return true;
#endif
}
