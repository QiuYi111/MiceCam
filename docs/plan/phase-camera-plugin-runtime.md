# Phase Plan: External Camera Plugin Runtime

**Date**: 2026-05-15  
**Status**: Proposed  
**Target**: camera backend runtime, plugin loader, source-grouped UI, recording pipeline  
**Depends On**:

- `docs/requirements/plugin-camera-backend-system.md`
- `docs/adr/0002-external-camera-plugin-runtime.md`
- `specs/003-camera-plugin-runtime/spec.md`

## Phase Overview

This phase migrates MiceCam from in-process camera backends to a plugin-only camera runtime. It is a core architecture change. All camera sources, including official FFmpeg and OAK, must be represented as external plugins. Final session outputs remain under MiceCam control.

## Phase 0: Contract Freeze

### Goal

Freeze the plugin protocol before implementation work fans out.

### Deliverables

- `camera_plugin.proto`
- manifest schema for `plugin.json`
- source/device/stream domain model
- shared memory ring descriptor contract
- error and recovery code registry
- resource request and allocation model

### Tasks

- Define gRPC services for handshake, discovery, capabilities, config, lifecycle, diagnostics, and stream control.
- Define frame descriptor and ring slot ownership semantics.
- Define requested output -> resolved output -> required postprocess negotiation.
- Define standard capability groups:
  - video stream capability
  - acquisition controls
  - scientific controls
  - plugin metadata
- Define config schema field types and `apply_mode`.

### Verification

- Proto/schema lint.
- Golden manifest validation tests.
- Contract tests using a fake plugin process.

### Exit Criteria

- Protocol and manifest are documented and testable.
- No implementation task is allowed to invent new protocol fields without updating this contract.

## Phase 1: Plugin Registry and Source Model

### Goal

Replace flat backend discovery with a source-grouped plugin registry.

### Deliverables

- plugin registry service
- bundled plugin discovery
- linked directory plugin config
- install-time validation flow
- source group model for QML

### Tasks

- Implement local config for linked plugin absolute paths.
- Implement `Import Plugin Directory`.
- Validate manifest and entrypoint.
- Run install-time handshake but require app restart before use.
- Build `CameraSourceModel`:
  - source id
  - source name
  - bundled or linked
  - devices
  - diagnostics
- Update UI to group cameras by plugin/source.

### Verification

- Import valid fake plugin directory.
- Reject invalid manifest.
- Missing plugin directory marks source `missing`.
- UI shows official and linked source groups.

### Exit Criteria

- No QML view depends on a flat raw camera backend list.
- Plugin registry failure leaves the app controllable.

## Phase 2: Official FFmpeg Plugin

### Goal

Move FFmpeg/AVFoundation USB camera discovery and capture out of the main app.

### Deliverables

- bundled official FFmpeg plugin executable
- FFmpeg plugin manifest
- macOS AVFoundation device discovery through a working mechanism
- capabilities and per-device config schema
- shared memory ring producer for raw/MJPEG/H264/H265 payloads as available

### Tasks

- Implement FFmpeg plugin gRPC server.
- Enumerate macOS AVFoundation devices using the working avfoundation listing path rather than unsupported `avdevice_list_input_sources`.
- Add device persistent ids where available.
- Implement output negotiation.
- Produce stream payload descriptors into MiceCam-allocated shared memory rings.

### Verification

- MacBook Pro Camera and iPhone Continuity Camera enumerate on target Mac.
- No-camera case reports zero devices with diagnostics.
- One stream can record through the plugin path.

### Exit Criteria

- In-process FFmpeg backend is no longer registered by the app.
- FFmpeg plugin path can produce valid MiceCam outputs.

## Phase 3: Recording Pipeline Consumer

### Goal

Make the existing recording pipeline consume plugin stream payloads while preserving fixed output artifacts.

### Deliverables

- plugin stream consumer
- shared memory ring reader
- raw -> H264/H265 encode path
- MJPEG -> H264/H265 transcode path
- H264/H265 packet remux path
- SRT/metadata/stats integration

### Tasks

- Map plugin frame descriptors into pipeline frame or packet inputs.
- Track sequence, PTS, keyframe, codec, and plugin metadata.
- Record backpressure and drops explicitly.
- Write plugin snapshot into `_meta.json`.
- Extend `_stats.json` for plugin transport stats.

### Verification

- Raw fake plugin stream records to MP4/SRT/meta/stats.
- MJPEG fake plugin stream transcodes to H264/H265 MP4.
- H264/H265 fake plugin stream remuxes or passthroughs.
- Dropped/backpressured frame is reflected in stats.

### Exit Criteria

- Plugin output cannot bypass session artifacts.
- Silent frame drop paths are covered by tests.

## Phase 4: Resource Manager

### Goal

Centralize plugin runtime allocation decisions.

### Deliverables

- process model chooser
- shared memory budget allocator
- stream and encoder budget planner
- exclusive resource lock manager
- recording vs preview backpressure policy

### Tasks

- Implement resource request evaluation.
- Allocate ring slot count and slot size.
- Enforce recording ring no-silent-drop policy.
- Enforce preview latest-frame/drop policy.
- Apply `exclusive_resource_id` conflict locks.

### Verification

- Conflicting devices cannot open simultaneously.
- Plugin preferred process model can be overridden by resource manager.
- Ring full behavior is explicit and observable.

### Exit Criteria

- MiceCam, not the plugin, owns recording-critical resource decisions.

## Phase 5: Official OAK Plugin

### Goal

Move OAK support to the official plugin path without blocking the current USB-only hardware gate.

### Deliverables

- bundled official OAK plugin executable
- OAK plugin manifest
- OAK capability schema
- OAK native H264 packet output path

### Tasks

- Implement DepthAI-backed plugin process.
- Enumerate OAK streams and persistent device ids.
- Emit H264/H265 compatible packet descriptors where supported.
- Surface SDK missing and device unavailable diagnostics.

### Verification

- Build and contract tests pass without OAK hardware.
- Hardware tests run when OAK is available.

### Exit Criteria

- OAK no longer requires in-process backend registration.
- Lack of OAK hardware does not block USB plugin release gate.

## Phase 6: Hardware Gate

### Goal

Prove the plugin runtime is production-worthy on currently available hardware.

### Hardware Matrix

- MacBook Pro Camera
- iPhone Continuity Camera

### Deliverables

- two-source USB/AVFoundation recording smoke test
- one-hour recording script
- artifact validation script
- plugin crash/disconnect test procedure

### Verification

- Two real video sources record for one hour.
- Outputs are H264/H265 `.mp4`, `.srt`, `_meta.json`, and `_stats.json`.
- MP4 files pass `ffprobe`.
- SRT timestamps are monotonic.
- No silent drops occur.
- Drop/backpressure metrics are explicit if the OS or camera drops frames.
- Stop/finalize completes successfully.

### Exit Criteria

- USB plugin runtime meets the production strict gate on two available devices.
- OAK remains a documented pending hardware validation item.

## Risk Classification

Blast radius: **core**.

This changes camera runtime architecture, process boundaries, IPC protocol, resource ownership, recording pipeline inputs, packaging, and UI device models. Implementation requires explicit staged review and cannot be treated as a leaf or branch patch.
