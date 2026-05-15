# Product Requirements: External Camera Plugin Backend System

**Date**: 2026-05-15  
**Status**: Draft  
**Target**: camera backend runtime, native UI device model, recording pipeline  
**Related Spec**: `specs/003-camera-plugin-runtime/spec.md`

## 1. Background

MiceCam currently has an internal camera backend abstraction, but new backend behavior still requires editing and recompiling the main application. The current FFmpeg/AVFoundation enumeration failure on macOS exposed the weakness: backend-specific discovery behavior is tightly coupled to the native app release cycle.

The product direction is to make MiceCam a host platform for camera backend plugins. Users and hardware vendors should be able to adapt special devices such as infrared cameras, microscopes, and OCT-like instruments without rebuilding MiceCam. The first version is not a marketplace and does not allow arbitrary UI or pipeline extensions. It is a strict external camera backend plugin runtime.

## 2. Product Goal

Deliver a production-grade external camera backend plugin system where all camera sources, including official FFmpeg and OAK backends, are discovered through plugins, configured through schema-driven UI, streamed through a MiceCam-owned shared memory transport, and recorded into the existing fixed MiceCam session outputs.

## 3. Primary Users

- Lab operators who need to select, configure, and record from multiple camera sources.
- Engineers adapting custom or vendor camera SDKs into MiceCam.
- Deployment owners distributing official and local camera backend plugins.

## 4. Core Decisions

1. First version supports camera backend plugins only. UI plugins, marketplace distribution, and pipeline extension plugins are out of scope.
2. Plugins are external processes.
3. IPC control plane uses gRPC.
4. Frame and packet payloads use a MiceCam-allocated shared memory ring from the first version.
5. gRPC streaming may carry descriptors and control events but is not the primary payload transport.
6. MiceCam owns final session outputs: H264/H265 `.mp4`, `.srt`, `_meta.json`, and `_stats.json`.
7. Plugins may emit raw frames, MJPEG packets, H264 packets, or H265 packets. MiceCam performs encode, remux, transcode, and final writing as needed.
8. Official FFmpeg and OAK backends are migrated to bundled external plugins. The main app has no in-process camera backend fallback.
9. User plugins are trusted local code. Plugin sandboxing, signing, and marketplace moderation are out of scope for the first version.
10. Plugin import uses linked local directories, not copied packages.
11. Plugin changes require application restart. Hot plugin reload is out of scope.
12. Device hotplug and runtime-safe configuration changes remain in scope.

## 5. Functional Requirements

### FR-1 Plugin Registry and Discovery

MiceCam must discover camera sources from:

- bundled official plugin directories inside the application distribution
- user-linked plugin directories stored in local app configuration

Each camera source must be represented by:

- source id
- source name
- source type: bundled or linked
- plugin path
- plugin version
- plugin API version
- enabled or disabled state
- diagnostics state

### FR-2 Plugin Manifest

Each plugin directory must contain `plugin.json` with:

- plugin id
- display name
- semantic version
- plugin API version
- minimum MiceCam version
- platform entrypoint
- required feature flags
- optional feature flags
- supported process models
- preferred process model

The first version stores plugin paths as absolute local paths and does not support migration across machines.

### FR-3 Import and Install-Time Validation

The UI must support importing a plugin by selecting a local directory.

On import, MiceCam must:

- validate `plugin.json`
- verify the current platform entrypoint exists and is executable
- reject path traversal and invalid plugin ids
- launch the plugin once for handshake and validation
- record the absolute path and enabled state in local app configuration
- mark the plugin as requiring application restart before use

### FR-4 Process Model

Plugins declare supported and preferred process models:

- singleton
- per-device
- per-stream

MiceCam has final scheduling authority based on platform, system state, user configuration, and resource budget.

### FR-5 gRPC Control Contract

The control plane must support at minimum:

- handshake
- get plugin info
- enumerate devices
- get capabilities
- get config schema
- validate config
- open stream
- start stream
- stop stream
- shutdown
- health/diagnostics

### FR-6 Shared Memory Frame Transport

Plugins declare memory requirements, but MiceCam allocates the shared memory ring.

Each stream ring must include:

- slot count
- slot size
- memory handle or platform descriptor
- stream id
- ownership rules
- producer/consumer sequence
- release/ack protocol

Recording transport must not silently overwrite or drop frames. Preview transport may use latest-frame semantics and drop frames by policy.

### FR-7 Output Normalization

MiceCam must write fixed outputs:

- one H264/H265 `.mp4` per selected stream
- one `.srt` annotation file per selected stream
- session `_meta.json`
- session `_stats.json`

Plugins must not write final session artifacts. Plugins may emit:

- raw frame payloads
- MJPEG encoded packets
- H264 encoded packets
- H265 encoded packets

MiceCam negotiates requested output, receives resolved output, and applies post-processing when needed.

### FR-8 Configuration Schema

Plugins expose schema-driven configuration. First version supports:

- string
- integer
- float
- boolean
- enum
- path
- conditional visibility
- dynamic schema per device
- apply mode: pre-open, pre-record, runtime-safe, requires-restart

Custom plugin UI controls are out of scope.

Configuration must support:

- plugin-level config
- per-device config keyed by stable persistent device id

### FR-9 Device Model and Grouped UI

The UI must group cameras by source/plugin:

- official FFmpeg plugin
- official OAK plugin
- linked user plugins

Each device must show:

- source/plugin name
- display name
- persistent id when available
- status
- diagnostics
- conflict/lock state

The user selects devices by default and may expand devices to select streams.

### FR-10 Conflict and Exclusive Resource Handling

Plugins may report `exclusive_resource_id`.

MiceCam must not automatically merge devices discovered by different plugins. If two devices declare the same exclusive resource id, opening one locks or disables the conflicting entries.

### FR-11 Resource Management

MiceCam owns the first-version resource manager for recording-critical resources:

- plugin process count
- shared memory budget
- stream count
- estimated input bandwidth
- estimated output bandwidth
- encoder slots
- disk bandwidth estimate
- preview budget
- exclusive device locks
- backpressure policy

USB topology, thermal, GPU DMA, and platform accelerator scheduling may be represented as future fields but are not hard constraints in the first version.

### FR-12 Error and Recovery Model

Plugin errors must be structured and include:

- code
- severity
- recoverable flag
- user message
- technical detail
- suggested action
- affected device or stream
- retry delay
- recovery action

Supported recovery actions include:

- none
- retry
- restart plugin
- re-enumerate devices
- disable device
- fallback config

### FR-13 Session Metadata Snapshot

Local plugin configuration is stored on the machine, but each session `_meta.json` must snapshot:

- plugin id
- plugin version
- plugin API version
- selected device persistent id
- resolved config
- capability snapshot
- required and optional features used
- resolved output and post-processing path

## 6. Non-Functional Requirements

### NFR-1 Reliability

Plugin crashes must not crash the UI process. Recording failure must leave MiceCam controllable and must produce structured diagnostics.

### NFR-2 Data Integrity

Recording transport must not silently drop frames. Any dropped, skipped, overwritten, or backpressured frame must be reflected in stats and annotations.

### NFR-3 Performance

First release must support two real USB/AVFoundation video sources for one hour with zero silent drops and valid H264/H265 MP4 outputs.

### NFR-4 Output Consistency

All plugin devices must produce the same final MiceCam artifact contract regardless of native camera codec.

### NFR-5 Maintainability

The native UI must consume source/device/stream models and must not branch on plugin implementation details.

## 7. Acceptance Criteria

1. Official FFmpeg backend runs as a bundled external plugin and enumerates MacBook Pro Camera and iPhone Continuity Camera on the target Mac.
2. Official OAK backend is packaged as a bundled external plugin, though OAK hardware is not required for the first hardware gate.
3. UI groups camera devices by plugin/source.
4. User-linked plugin directories can be imported, validated, enabled, disabled, and marked restart-required.
5. Per-device configuration persists by plugin id plus persistent device id.
6. Two real video sources record for one hour through the plugin runtime.
7. Outputs are H264/H265 `.mp4`, `.srt`, `_meta.json`, and `_stats.json`.
8. No silent frame drops occur; drops or backpressure are observable in `_stats.json`.
9. Plugin crash, plugin startup failure, missing plugin path, and no-device states leave the UI controllable.
10. The main application has no in-process camera backend fallback.

## 8. Out of Scope

- Online plugin marketplace.
- Plugin signing, sandboxing, malware scanning, or permission prompts.
- Custom plugin-rendered UI controls.
- Plugin-defined final file layouts.
- Plugin-written MP4/SRT/session artifacts.
- Hot plugin reload without application restart.
- Project-level portable plugin configuration.
- OCT-specific volume reconstruction pipeline.
- GPU/DMA zero-copy implementation beyond protocol reservation.
