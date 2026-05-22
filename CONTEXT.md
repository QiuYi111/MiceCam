# MiceCam

Multi-camera animal monitoring system with OAK-D spatial AI cameras. C++ SDK with Python bindings for real-time video capture, recording, and remote viewing.

## Architecture

The system follows a **supervisor/worker** architecture (ADR 0001): the UI process acts as a supervisor that manages an external worker process responsible for recording runtime. Camera backends run as **external plugins** (ADR 0002), communicating via gRPC control plane and shared-memory ring buffers for frame data.

## Language

- **Camera Source** — a logical group of physical camera devices backed by a single plugin. One plugin may expose multiple sources.
- **Plugin** — an external process that provides camera enumeration, configuration, and frame capture for one or more devices. Communicates via gRPC + shared memory rings.
- **Recording Pipeline** — the end-to-end flow from frame capture through encoding (H264/H265), muxing (MP4), SRT subtitle generation, and session metadata/stats writing.
- **Plugin Manifest** — `plugin.json` in each plugin directory declaring id, version, API version, feature flags, and process model.
- **Process Model** — how a plugin instantiates: singleton (one process for all devices), per-device, or per-stream.
- **Shared Memory Ring** — a lock-free ring buffer allocated by the host and accessed by the plugin for zero-copy frame/packet delivery.
- **Ring Descriptor** — metadata describing a shared memory ring: name, slot count, slot size, payload kind.
- **Stream** — a single video/data flow from one camera device. Has configuration (resolution, framerate, codec) and produces frames or encoded packets.
- **Supervisor** — the UI process that manages worker lifecycle, surfaces state, and handles crash recovery. Does not own the recording runtime.
- **Worker** — the external process running the recording engine: capture, encode, mux, and artifact writing.
- **Liveness Monitor** — tracks plugin health via gRPC channel state and heartbeat, escalating to crash recovery on stall or disconnect.
- **Session** — one recording run, producing a directory of artifacts: `.mp4`, `.srt`, `_meta.json`, `_stats.json`.
- **Payload Kind** — the format of data in a ring slot: raw frame, MJPEG packet, H264 packet, or H265 packet.
- **Plugin Registry** — the authoritative index of discovered plugins, their manifests, enabled/disabled state, and per-plugin source/device catalog.

## Relationships

- A **Plugin** exposes one or more **Camera Sources**.
- A **Camera Source** contains one or more **Streams** (one per physical camera device).
- The **Supervisor** manages the **Worker** lifecycle via IPC commands and receives structured state/event messages.
- The **Worker** owns the **Recording Pipeline** and communicates with **Plugins** for frame capture.
- **Ring Descriptors** are exchanged during stream negotiation; the plugin writes into rings the host reads from.
- **Plugin Registry** is the single source of truth for plugin discovery and state; all other modules consume it.

## Example dialogue

```
"Each Camera Source is backed by one Plugin. When the Supervisor starts a Session, it
sends a stream negotiation request to the Worker, which opens the Plugin's gRPC
stream and exchanges Ring Descriptors for shared-memory transport."

"The Liveness Monitor detects that the FFmpeg Plugin's gRPC channel went idle. It
fires a crash alert to the Supervisor, which transitions the UI state to recovering
and offers the operator restart or terminal-failure options."
```

## Flagged ambiguities

- `internal/micecam/` contains an empty Python package and egg-info — likely vestigial. Actual Python bindings are expected in a `bindings/` directory per ADR 0002.
- Two `main.cpp` files exist (`cmd/micecam_ui/` and `cmd/micecam_v2/`). `micecam_v2` is a dead QML shell superseded by `micecam_ui`.
- `CameraManager` and `PluginRegistry` (domain) duplicate `PluginRegistryService` (infrastructure) functionality. ADR 0002 should resolve this.
