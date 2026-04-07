# Implementation Report: Native App Windows Packaging Workflow

**Date**: 2026-03-11 20:00
**Branch**: `codex/windows-native-packaging`
**Scope**: GitHub Actions packaging support for the pure C++ `micecam_ui` Windows artifact

## Summary

Updated the existing GitHub Actions workflow so the Windows `native-app` artifact is a runnable packaged bundle rather than a bare `micecam_ui.exe`.

## Changes

- Added a Windows-only deployment step in `.github/workflows/ci.yml`.
- Located `windeployqt.exe` from the vcpkg-installed Qt toolchain during CI packaging.
- Ran `windeployqt --release --compiler-runtime` against `dist/native-app/micecam_ui.exe`.
- Copied runtime DLLs from the active Windows vcpkg triplet into `dist/native-app/`.
- Copied build-produced DLLs into `dist/native-app/` to avoid missing non-Qt runtime dependencies.
- Strengthened packaged-bundle validation by asserting `platforms/qwindows.dll` exists before the worker handshake check.
- Updated `docs/wikis/native-app-release-checklist.md` to document the Windows deployment gate explicitly.

## Rationale

The previous workflow only copied `build/micecam_ui.exe` into the artifact directory on Windows. That output was not a real package because it omitted:

- Qt platform/runtime deployment
- compiler runtime deployment
- dynamic libraries produced or consumed through the vcpkg-backed build

That behavior violated the native app release requirement that packaging validation be part of the release gate.

## Verification

- Performed a workflow-level self-review against the packaging requirements in `docs/requirements/native-app-production-integration.md`.
- Confirmed the updated workflow now validates both:
  - Windows Qt platform plugin deployment
  - packaged worker handshake behavior

## Risks / Follow-up

- The workflow currently copies all DLLs from the selected Windows vcpkg triplet `bin/` directory. This is pragmatic and robust for CI artifacts, but it may be larger than a minimal distributable package.
- If the project later introduces an installer format or CMake install/deploy target for `micecam_ui`, the workflow should switch to that single authoritative packaging path.
