# Native App Release Checklist

## Purpose

This checklist defines the minimum local release gate for the native Qt/QML application while the packaging path is still stabilizing.

## Build Gate

Run from a clean checkout unless intentionally validating an in-progress branch:

```bash
cmake -S . -B build -DMICECAM_BUILD_TESTS=ON
cmake --build build --target micecam_tests micecam_ui -j4
```

## Verification Gate

Required checks:

```bash
ctest --test-dir build --output-on-failure
make lint
scripts/smoke_native_app.sh
```

## Manual Native UI Smoke

Validate the following in the desktop UI:

1. App launches and shows the redesigned workspace without QML load errors.
2. Camera inventory refreshes and unsupported settings are blocked before start.
3. Starting a session transitions through ready -> launching worker -> recording.
4. Stopping a session transitions through stopping -> decoding/completed.
5. Completed and error states expose output paths and the open-output action.
6. Closing while decode is in progress does not freeze the UI.

## Packaging Gate

Before promoting a packaged build:

1. Verify the packaged app bundle includes the worker-capable `micecam_ui` binary.
2. Launch the packaged app and repeat the manual native UI smoke flow.
3. Run the worker handshake against the packaged binary if the packaging format allows headless invocation.
4. Confirm Qt runtime dependencies resolve on a clean target machine.

## Known Current Limitation

The worker-process runtime currently prioritizes recording isolation over live preview transport.

Expected behavior in this release slice:

- recording, stop, decode, and error mapping are active
- preview is best-effort, capped, and may drop frames before recording is allowed to destabilize
