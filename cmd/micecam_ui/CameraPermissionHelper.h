#pragma once

/// Request camera access permission on platforms that require it.
/// On macOS, this triggers the system permission dialog if not yet granted.
/// Returns true if access is granted or not required.
extern "C" bool micecam_request_camera_access();
