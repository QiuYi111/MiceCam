#pragma once

/// Request camera access permission on platforms that require it.
/// On macOS, this triggers the system permission dialog if not yet granted.
/// On other platforms, returns true (permission not required or handled by OS).
/// Returns true if access is granted or not required.
bool micecam_request_camera_access();
