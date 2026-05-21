#include "NativeCameraEnumerator.h"

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

namespace micecam::infrastructure {

std::vector<NativeCameraInfo> enumerate_native_cameras() {
    std::vector<NativeCameraInfo> result;
    @autoreleasepool {
        AVCaptureDeviceDiscoverySession* session =
            [AVCaptureDeviceDiscoverySession
                discoverySessionWithDeviceTypes:@[
                    AVCaptureDeviceTypeExternal,
                    AVCaptureDeviceTypeBuiltInWideAngleCamera
                ]
                mediaType:AVMediaTypeVideo
                position:AVCaptureDevicePositionUnspecified];

        for (AVCaptureDevice* device in session.devices) {
            NativeCameraInfo info;
            info.id   = device.uniqueID.UTF8String;
            info.name = device.localizedName.UTF8String;
            result.push_back(info);
        }
    }
    return result;
}

} // namespace micecam::infrastructure
