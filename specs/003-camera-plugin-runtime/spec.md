# Feature Spec: External Camera Plugin Runtime

## Metadata

| Field      | Value |
|------------|-------|
| Feature ID | `003-camera-plugin-runtime` |
| Branch     | `codex/camera-plugin-runtime` |
| Status     | Draft |
| Owner      | `jingyi` |
| Date       | `2026-05-15` |

## Summary

MiceCam will replace in-process camera backends with a plugin-only external camera runtime. Official FFmpeg and OAK backends become bundled plugins, user camera backends are linked from local directories, control uses gRPC, payloads use MiceCam-allocated shared memory rings, and MiceCam remains responsible for fixed H264/H265 MP4, SRT, metadata, and stats outputs. The first hardware gate targets two real USB/AVFoundation video sources with one-hour recording and zero silent drops.

## User Scenarios

### US-001: Import Linked Camera Plugin Directory

**Priority**: P1

**Independent Test**: Create a fake plugin directory containing `plugin.json` and a test plugin executable. In the app, select Import Plugin Directory. Verify MiceCam validates the manifest, performs handshake, stores the absolute path, marks the plugin enabled, and shows "restart required".

**Acceptance Scenarios**:
- Given a valid plugin directory, When the user imports it, Then MiceCam validates manifest, entrypoint, and handshake before registering it.
- Given a plugin directory is missing `plugin.json`, When the user imports it, Then MiceCam rejects it with a structured diagnostic.
- Given an imported plugin directory is later deleted, When MiceCam starts, Then the plugin is marked missing and the UI remains controllable.

### US-002: Discover Cameras by Source Group

**Priority**: P1

**Independent Test**: Launch MiceCam with official FFmpeg plugin enabled on the target Mac. Verify the camera view shows an official FFmpeg source group containing MacBook Pro Camera and iPhone Continuity Camera.

**Acceptance Scenarios**:
- Given official FFmpeg plugin is bundled, When MiceCam starts, Then the plugin is launched and listed as a camera source.
- Given FFmpeg plugin enumerates MacBook Pro Camera and iPhone Continuity Camera, When the camera grid renders, Then devices appear under the FFmpeg source group.
- Given no plugin reports devices, When the camera grid renders, Then the UI shows an empty source/device state and recording remains disabled.

### US-003: Configure Device Through Schema

**Priority**: P1

**Independent Test**: Use a fake plugin that exposes per-device config fields for exposure, gain, ROI, and codec preference. Open the device detail page. Verify fields render from schema, values persist by plugin id plus persistent device id, and invalid values are rejected by `validate_config`.

**Acceptance Scenarios**:
- Given a plugin returns a dynamic config schema for a device, When the device detail page opens, Then MiceCam renders standard controls for supported field types.
- Given a field has `apply_mode=runtime_safe`, When recording is active, Then the field remains editable and changes are logged to session metadata/events.
- Given a field has `apply_mode=pre_record`, When recording is active, Then the field is disabled until recording stops.

### US-004: Record Raw Plugin Stream

**Priority**: P1

**Independent Test**: Run a fake plugin that produces raw frames through a MiceCam-allocated shared memory ring. Record 10 seconds. Verify output H264/H265 MP4, SRT, `_meta.json`, and `_stats.json`.

**Acceptance Scenarios**:
- Given a plugin stream resolves to raw frames, When recording starts, Then MiceCam encodes frames into H264/H265 MP4.
- Given each raw frame has a descriptor with sequence and PTS, When recording completes, Then SRT entries are monotonic and stats include frame counts.

### US-005: Record Encoded Plugin Stream

**Priority**: P1

**Independent Test**: Run fake plugins that emit MJPEG and H264 packet streams. Verify MJPEG is transcoded to H264/H265 MP4 and H264/H265 packets are remuxed or passed through into MP4 while SRT/meta/stats are still written by MiceCam.

**Acceptance Scenarios**:
- Given a plugin stream resolves to MJPEG packets, When recording starts, Then MiceCam transcodes to H264/H265 MP4.
- Given a plugin stream resolves to H264/H265 packets compatible with MP4, When recording starts, Then MiceCam writes final MP4 without plugin-written artifacts.
- Given requested H265 is unavailable, When the plugin resolves output to H264, Then the UI and metadata show the resolved output and post-processing path.

### US-006: Enforce Shared Memory Ownership and Backpressure

**Priority**: P1

**Independent Test**: Run a fake plugin that fills the shared memory ring faster than MiceCam consumes it. Verify recording path does not silently overwrite frames and `_stats.json` records backpressure or dropped frame events explicitly.

**Acceptance Scenarios**:
- Given the recording ring is full, When the plugin attempts to submit another payload, Then MiceCam blocks, rejects, or records backpressure according to policy.
- Given the preview ring is full, When new preview payloads arrive, Then MiceCam may drop preview frames using latest-frame semantics.

### US-007: Handle Plugin Failures

**Priority**: P1

**Independent Test**: Use a plugin that fails handshake, crashes during enumeration, and crashes during recording. Verify UI diagnostics, structured errors, and recovery actions.

**Acceptance Scenarios**:
- Given a plugin fails handshake, When MiceCam starts, Then the plugin source is disabled with diagnostic code and message.
- Given a plugin crashes during recording, When the crash is detected, Then MiceCam surfaces the affected source/device and preserves application control.
- Given a plugin reports `DEVICE_BUSY`, When the device row renders, Then the UI shows the source as unavailable with suggested action.

### US-008: Validate Two-USB Hardware Gate

**Priority**: P1

**Independent Test**: On the target Mac, record MacBook Pro Camera and iPhone Continuity Camera for one hour through the official FFmpeg plugin path. Verify outputs and stats.

**Acceptance Scenarios**:
- Given two real USB/AVFoundation video sources are available, When recording runs for one hour, Then both streams produce valid H264/H265 MP4 and SRT files.
- Given the OS or device drops frames, When the session finishes, Then drops are reported in `_stats.json` and not silent.
- Given recording stops, When finalization completes, Then `_meta.json` contains plugin version, capability snapshot, resolved config, and resolved output.

## Requirements

### Functional Requirements

- **FR-001**: MiceCam must load camera sources only from external plugins; in-process camera backend fallback is not allowed.
- **FR-002**: Official FFmpeg and OAK backends must be packaged as bundled external plugins.
- **FR-003**: User plugins must be linked from local directories stored as absolute paths.
- **FR-004**: Plugin import must validate manifest, entrypoint, API compatibility, feature flags, and handshake.
- **FR-005**: Plugin import, enable, disable, and update require application restart before runtime use.
- **FR-006**: Plugin control plane must use gRPC.
- **FR-007**: Frame and packet payloads must use MiceCam-allocated shared memory rings.
- **FR-008**: Plugins must declare stream memory requirements; MiceCam must allocate ring resources.
- **FR-009**: Plugins may emit raw frames, MJPEG packets, H264 packets, or H265 packets.
- **FR-010**: MiceCam must negotiate requested output, resolved output, and required post-processing with each plugin stream.
- **FR-011**: MiceCam must write final H264/H265 MP4, SRT, `_meta.json`, and `_stats.json`; plugins must not write final session artifacts.
- **FR-012**: The UI must group devices by camera source/plugin.
- **FR-013**: Built-in official plugins and linked user plugins must use the same source/device/stream model.
- **FR-014**: Device selection must default to device-level selection and allow stream-level advanced selection.
- **FR-015**: Plugin config must support plugin-level and per-device configuration.
- **FR-016**: Per-device config must be keyed by plugin id and stable plugin-provided persistent device id.
- **FR-017**: Plugins without stable persistent ids may enumerate and record but cannot persist per-device config without warning.
- **FR-018**: Config schema must support simple field types, conditional visibility, dynamic per-device schema, and apply modes.
- **FR-019**: MiceCam must implement structured plugin diagnostics with error code, severity, recoverability, user message, technical detail, suggested action, affected target, retry delay, and recovery action.
- **FR-020**: MiceCam must support optional `exclusive_resource_id` conflict locks without automatically merging duplicate devices.
- **FR-021**: MiceCam resource manager must own process model selection, shared memory budget, stream count, encoder assignment, disk bandwidth estimate, preview budget, exclusive locks, and backpressure policy.
- **FR-022**: Recording payload transport must not silently drop or overwrite frames.
- **FR-023**: Preview payload transport may drop frames using explicit latest-frame policy.
- **FR-024**: Session metadata must snapshot plugin id, version, API version, selected persistent device id, resolved config, capability snapshot, features, resolved output, and post-processing path.

### Non-Functional Requirements

- **NFR-001**: Reliability - Plugin process crash must not crash the UI process.
- **NFR-002**: Data integrity - Recording must have zero silent drops; any drop/backpressure must be represented in stats.
- **NFR-003**: Performance - Two real USB/AVFoundation video sources must record for one hour through the plugin runtime.
- **NFR-004**: Output compatibility - Outputs must be H264/H265 MP4 plus SRT/meta/stats regardless of input codec.
- **NFR-005**: Startup behavior - Plugin loader failure must leave the app usable with recording disabled and diagnostics visible.
- **NFR-006**: Maintainability - QML must bind to source/device/stream models and must not branch on plugin implementation details.
- **NFR-007**: Observability - Plugin lifecycle, transport, backpressure, and recovery events must be structured and searchable in logs/stats.

## Success Criteria

| # | Criterion | Measured By |
|---|-----------|-------------|
| SC-1 | FFmpeg official plugin enumerates MacBook Pro Camera and iPhone Continuity Camera | Hardware smoke on target Mac |
| SC-2 | UI groups devices by source/plugin | QML model test or manual screenshot verification |
| SC-3 | Linked plugin directory import validates manifest and handshake | Fake plugin integration test |
| SC-4 | Raw plugin stream records to H264/H265 MP4 + SRT/meta/stats | Fake plugin recording test |
| SC-5 | MJPEG plugin stream transcodes to H264/H265 MP4 | Fake MJPEG packet test |
| SC-6 | H264/H265 plugin packet stream writes final MP4 without plugin artifacts | Fake encoded packet test |
| SC-7 | Ring backpressure is observable and not silent | Stress test with overproducing fake plugin |
| SC-8 | Plugin crash leaves UI controllable and diagnostics visible | Fault injection test |
| SC-9 | Two real USB/AVFoundation sources record for one hour | Hardware validation script |
| SC-10 | Session metadata snapshots plugin/capability/config/output data | `_meta.json` inspection |

## Assumptions

- User plugins are trusted local code.
- Plugin developers can implement gRPC and shared memory protocol.
- First target hardware gate has two physical video sources: MacBook Pro Camera and iPhone Continuity Camera.
- OAK hardware is not available for the first gate and does not block the USB plugin release gate.
- Final output remains video-centric. OCT-specific volume reconstruction is deferred.
- Local absolute plugin paths are acceptable and do not need migration across machines.
- Application restart after plugin import/update/enable/disable is acceptable.

## Clarifications

- None currently. The grill session resolved the first-version scope.

## Out of Scope

- Online plugin marketplace.
- Plugin signing, sandboxing, malware scanning, or permission policy.
- Custom plugin UI controls.
- Plugin-defined final session file layout.
- Plugin-written MP4/SRT/meta/stats artifacts.
- Hot plugin reload without application restart.
- Project-level portable plugin config.
- In-process camera backend fallback.
- OCT volume reconstruction or arbitrary pipeline extensions.
- Full GPU/DMA zero-copy implementation beyond protocol reservation.
- Three-or-more-camera hardware gate until hardware is available.

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Shared memory ring complexity causes cross-platform bugs | High | High | Freeze descriptor contract first; implement fake plugin stress tests before FFmpeg migration |
| Plugin-only path breaks all camera access if loader fails | Medium | High | App remains usable; Plugin Manager and diagnostics must be accessible without cameras |
| FFmpeg official plugin still fails AVFoundation discovery | Medium | High | Use CLI-equivalent discovery path or platform API bridge; add target Mac smoke test |
| Two-source one-hour hardware gate exposes OS camera contention | Medium | Medium | Treat device busy and Continuity disconnect as structured diagnostics |
| Agent parallel implementation diverges from protocol | High | High | Contract freeze phase; no worker may invent protocol fields without spec/ADR update |
| Removing in-process fallback makes rollback harder | Medium | High | Keep migration branch isolated until plugin hardware gate passes |
