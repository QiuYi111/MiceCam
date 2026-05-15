# ADR 0002: Use External Camera Backend Plugins as the Only Camera Runtime Path

**Date**: 2026-05-15  
**Status**: Proposed  
**Decision Makers**: Product and architecture planning for MiceCam v2 plugin runtime

## Context

MiceCam currently has a camera backend abstraction, but adding or fixing a backend still requires rebuilding the main application. The macOS AVFoundation enumeration issue showed that backend-specific behavior can block the UI from discovering real cameras even when the operating system and FFmpeg CLI can see them.

The product direction is to let users and hardware vendors adapt special cameras without changing the MiceCam application. Target devices include USB cameras, OAK devices, infrared cameras, microscopes, and future scientific imaging hardware. The first plugin version is intentionally limited to camera backend plugins. It does not support arbitrary UI plugins or arbitrary pipeline extensions.

ADR 0001 already requires a process boundary for recording runtime reliability. This ADR extends that direction to camera backend integration: camera SDKs and device-specific logic should run outside the UI process and should not share a failure domain with the native app shell.

## Decision

MiceCam will use external camera backend plugins as the only camera runtime path.

Official FFmpeg and OAK backends will be migrated into bundled external plugins. User camera backends will be linked from local directories. The main application will not retain an in-process FFmpeg/OAK camera backend fallback.

The plugin runtime will use:

- gRPC for the control plane
- MiceCam-allocated shared memory rings for frame and packet payloads
- source-grouped camera models for UI
- schema-driven plugin and device configuration
- fixed final MiceCam outputs: H264/H265 `.mp4`, `.srt`, `_meta.json`, and `_stats.json`

## Decision Drivers

- Users need to adapt special camera hardware without rebuilding MiceCam.
- Backend failures must not crash the UI process.
- Official backends should exercise the same plugin path as user plugins.
- MiceCam must preserve unified recording outputs and data integrity.
- High-throughput multi-camera recording requires a low-copy data path.
- UI should show devices grouped by source/plugin rather than a flat ambiguous list.

## Architecture

### Process Shape

```text
MiceCam UI / Supervisor
  Plugin Registry
  Resource Manager
  Camera Source Model
  Recording Pipeline
  MP4/SRT/metadata/stats writers
    |
    | gRPC control
    | shared memory ring descriptors
    v
External Camera Plugin Process
  device SDK
  enumerate/capabilities/config
  frame or encoded packet producer
```

### Camera Source Types

All camera sources are logical plugins:

- bundled official plugin: `micecam.ffmpeg`
- bundled official plugin: `micecam.oak`
- linked user plugin: vendor-defined plugin id

The UI consumes `CameraSource` groups and does not special-case official backends.

### Plugin Manifest

Every plugin directory contains `plugin.json`.

Manifest fields include:

- `id`
- `name`
- `version`
- `plugin_api_version`
- `min_micecam_version`
- platform entrypoints
- required feature flags
- optional feature flags
- supported process models
- preferred process model

### Process Model

Plugins declare supported and preferred process models:

- singleton
- per-device
- per-stream

MiceCam has final scheduling authority. It may choose any plugin-supported process model based on platform, system state, user configuration, and resource budget.

### Configuration

Plugins expose schema-driven configuration. The first version supports simple fields, conditional visibility, dynamic per-device schemas, and apply modes:

- pre-open
- pre-record
- runtime-safe
- requires-restart

Configuration is local to the machine. Plugin directories are stored as absolute paths. Per-device configuration is keyed by plugin id plus a stable plugin-provided persistent device id.

### Data Transport

Plugins declare memory requirements for each stream. MiceCam allocates shared memory rings and returns ring descriptors to the plugin.

Payload kinds:

- raw frame
- MJPEG encoded packet
- H264 encoded packet
- H265 encoded packet

Recording rings must not silently overwrite or drop frames. Preview rings may use latest-frame semantics.

### Output Contract

Plugins do not write final artifacts.

MiceCam writes:

- H264/H265 `.mp4`
- `.srt`
- `_meta.json`
- `_stats.json`

MiceCam negotiates requested output with the plugin. The plugin returns resolved output and required post-processing. MiceCam performs encode, remux, transcode, and final writing as needed.

## Consequences

### Positive

- Backend implementation can evolve independently of MiceCam core.
- Official and user backends exercise one consistent runtime path.
- Camera SDK crashes are contained to plugin processes.
- UI can show camera source grouping and diagnostics consistently.
- Shared memory transport supports high-throughput raw video paths.
- Fixed outputs preserve downstream analysis workflows.

### Negative

- This is a core architecture change with a broad test surface.
- Packaging becomes more complex because official plugins must ship with the app.
- gRPC and shared memory protocol design is now mandatory.
- No in-process fallback means plugin loader failures can disable all camera access.
- Official FFmpeg and OAK code must be migrated rather than wrapped in place.

## Rejected Alternatives

### In-Process Dynamic Library Plugins

Rejected because user/vendor camera SDK crashes would share the UI process failure domain. ABI stability and cross-platform dependency management would also be fragile.

### Keep Official Backends In-Process

Rejected because it creates two runtime paths and prevents official backends from validating the plugin system.

### Plugin-Written Session Artifacts

Rejected because it would fragment the output contract and weaken session integrity, annotation, metadata, and stats guarantees.

### gRPC-Only Frame Streaming

Rejected for the first production target because multi-camera raw video requires a low-copy payload path. gRPC remains the control plane.

## Compatibility With ADR 0001

This ADR is compatible with ADR 0001. The native UI remains a supervisor, recording-critical work remains outside the UI process, and failures are surfaced through structured state and diagnostics.

## Open Follow-Ups

1. Define `camera_plugin.proto`.
2. Define shared memory ring descriptor format and platform handle passing.
3. Define plugin registry configuration file.
4. Define official FFmpeg plugin package layout.
5. Define hardware validation scripts for two USB/AVFoundation sources.
