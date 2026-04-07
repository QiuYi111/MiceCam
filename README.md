# MiceCam

MiceCam is a mixed C++/Python camera capture system for behavioral recording. The repository currently contains:

- a native Qt/QML desktop app in `cmd/micecam_ui/`
- a legacy packaged Python/PyQt app in `cmd/gui/`
- a shared C++ capture/recording core in `internal/`
- pybind11 bindings in `bindings/python/`

## Current shipping paths

- Native app source entry: `cmd/micecam_ui/main.cpp`
- Native UI state bridge: `cmd/micecam_ui/PipelineController.*`
- Legacy packaged desktop app: `cmd/gui/micecam_app.py`
- Legacy packaged worker: `cmd/gui/recorder_worker.py`
- Shared C++ pipeline core: `internal/infrastructure/`

Start with [project_index](D:/MiceCam/project_index) if you need a fast repo map.

## Repo layout

```text
api/        Public headers and contracts
bindings/   Python bindings for the native core
cmd/        Desktop apps, workers, examples
docs/       Requirements, ADRs, plans, reports, wikis
internal/   Core C++ implementation
legacy/     Historical SDK/release artifacts kept for reference
tests/      Native tests
```

## Build paths that actually matter

### 1. Native Qt/QML app

Use this when working on the native app in `cmd/micecam_ui/`.

Typical Windows configure:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
  -S . `
  -B build-native-win230-prebuilt `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=D:/MiceCam/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DMICECAM_BUILD_TESTS=ON `
  -DMICECAM_USE_PREBUILT_DEPTHAI=ON
```

Typical build:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
  --build build-native-win230-prebuilt `
  --config Debug `
  --target micecam_ui
```

### 2. Python bindings for the packaged Python app

Use this when rebuilding `_micecam` for `cmd/gui/` packaging:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
  -S . `
  -B build-python-package `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=D:/MiceCam/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DMICECAM_USE_PREBUILT_DEPTHAI=ON `
  -DBUILD_PYTHON_BINDINGS=ON `
  -DBUILD_QT_UI=OFF `
  -DMICECAM_BUILD_TESTS=OFF `
  -DBUILD_TESTS=OFF `
  -DPython3_EXECUTABLE=C:/Users/25138/AppData/Roaming/uv/python/cpython-3.14.0-windows-x86_64-none/python.exe `
  -DPython3_ROOT_DIR=C:/Users/25138/AppData/Roaming/uv/python/cpython-3.14.0-windows-x86_64-none `
  -DPython3_INCLUDE_DIR=C:/Users/25138/AppData/Roaming/uv/python/cpython-3.14.0-windows-x86_64-none/include `
  -DPython3_LIBRARY=C:/Users/25138/AppData/Roaming/uv/python/cpython-3.14.0-windows-x86_64-none/libs/python314.lib
```

Then:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" `
  --build build-python-package `
  --config Release `
  --target _micecam
```

### 3. Windows MSI packaging

The MSI path currently packages the legacy Python app, not the native QML app.

Full build:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_windows.ps1
```

If `_micecam` is already rebuilt and you only want to repack:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_windows.ps1 -SkipNativeBuild
```

Installer output:

- `dist/MiceCam-Installer.msi`

## Verification

Useful native targets:

```powershell
cmake --build build-native-win230-prebuilt --config Debug --target micecam_ui
cmake --build build-native-win230-prebuilt --config Debug --target micecam_tests
```

Known current limitation on this machine:

- `micecam_tests.exe` may build successfully but test discovery can still fail with Windows runtime error `0xc0000135`

## Persistent Windows pitfalls

Read this before spending time on packaging or camera enumeration:

- [windows-dev-pitfalls.md](D:/MiceCam/docs/wikis/windows-dev-pitfalls.md)

Highlights:

- Use the same FFmpeg enumeration path in the UI and worker for USB cameras on Windows.
- Do not write worker status files to relative paths when running from `Program Files`.
- The local `.venv` Python may not provide the development artifacts needed by CMake; the `uv` CPython install does.
- PyInstaller does not reliably pull `depthai-core.dll` automatically from `_micecam` dependencies.
- Native preview can look "wired up" while staying blank if observer dispatch is not happening in `IngestionPipeline`.

## More docs

- Native runtime architecture: [native-app-runtime-architecture.md](D:/MiceCam/docs/wikis/native-app-runtime-architecture.md)
- Native release checklist: [native-app-release-checklist.md](D:/MiceCam/docs/wikis/native-app-release-checklist.md)
- Native app requirements: [native-app-production-integration.md](D:/MiceCam/docs/requirements/native-app-production-integration.md)

## Status note

The repo contains active Windows/native-app work and an existing modified `3rdParty/depthai-core` submodule state. Treat submodule changes carefully and do not assume a pristine vendor baseline.
