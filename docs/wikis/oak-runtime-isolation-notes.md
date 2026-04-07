# OAK Runtime Isolation Notes

## Summary

Windows OAK bring-up with the provided prebuilt `depthai-core-v3.4.0-win64` is sensitive to compilation and linkage boundaries.

The same four-camera synchronized MJPEG pipeline can succeed in a standalone executable while failing when compiled into a reusable project library.

This means the safest production architecture is to isolate DepthAI runtime ownership behind a very small OAK-specific boundary.

## Verified Findings

### 1. Hardware and prebuilt DepthAI are not the primary failure

The following were verified with standalone probes:

- device boot succeeds
- camera discovery succeeds
- single-camera frame delivery succeeds
- hardware sync succeeds
- quad `Sync + VideoEncoder + MessageGroup` succeeds

This rules out the earlier assumption that the board, USB transport, or prebuilt `depthai-core` package were fundamentally unusable on Windows.

### 2. `Invalid Member Count (buildPipeline)` was caused by project-side integration context

A standalone probe using only DepthAI headers and a local pipeline implementation succeeded.

The same logical pipeline failed inside the project backend path with:

- `Invalid Member Count (buildPipeline)`

After reducing project header exposure and ensuring `depthai/depthai.hpp` was included before project headers in the runtime compilation unit, that failure disappeared.

The working conclusion is that the original failure was caused by compilation-unit contamination from project headers or integration context, not by the pipeline logic itself.

### 3. The remaining backend bug was adapter lifecycle, not pipeline logic

After the pipeline-build issue was resolved, the remaining backend failure:

- `Couldn't open stream`

was traced to adapter lifecycle logic rather than to the runtime session itself.

The specific cause was an unconditional `stop()` at the start of `OAKCameraBackend::initialize()`. On Windows with the prebuilt DepthAI runtime, that premature stop path could break the first stream open. Restricting `stop()` to the case where a prior session actually exists fixed the backend initialization path.

Windows recording shutdown also needed a small platform-specific fix:

- unbuffered writes still need a buffered final truncate step if the logical payload size is not sector aligned

## Architectural Implication

The most robust shape for Windows OAK support is:

1. A dedicated OAK runtime boundary that owns all DepthAI interaction.
2. A plain data contract exposed to the rest of the application.
3. No direct leakage of `dai::*` types into UI, business, or generic camera layers.

Preferred production form:

- `oak_backend.exe` as a dedicated capture process

Currently verified workable form:

- `micecam_oak_runtime` as a dedicated runtime library consumed by a very thin adapter layer

## Recommended Data Contract

The rest of the application should consume a transport-neutral payload such as:

- `OAKEncodedFrame`
- `OAKFrameGroup`

Each frame should contain:

- sequence id
- width
- height
- encoded MJPEG bytes

No `dai::ImgFrame`, `dai::MessageGroup`, or DepthAI queue types should cross the runtime boundary.

## Recommended For Native App Planning

Native app development can resume if OAK is treated as an isolated subsystem rather than a normal in-process camera backend.

Recommended gating rule:

- do not restart full native app feature work on the assumption that OAK is just another linked backend

Recommended restart condition:

- resume native app integration through the isolated runtime boundary and keep all new work off the old mixed backend shape
