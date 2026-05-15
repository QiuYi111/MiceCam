# Worker Report: Phase 1 — Plugin Registry and Source Model

## Summary

Implemented the plugin registry runtime infrastructure: bundled plugin discovery from `3rdParty/bundled_plugins/`, linked plugin directory import/validation, persistent plugin config management, a source-grouped Qt model for QML, and wiring through CameraManager → AppController.

## Changed Files

### New files (7)

| File | Lines | Description |
|------|-------|-------------|
| `internal/infrastructure/PluginRegistryService.h` | 53 | Core plugin discovery service header |
| `internal/infrastructure/PluginRegistryService.cpp` | 212 | Discovery, validation, enable/disable, diagnostics |
| `internal/infrastructure/LinkedPluginConfig.h` | 24 | Persistent linked plugin config header |
| `internal/infrastructure/LinkedPluginConfig.cpp` | 71 | JSON file read/write for linked plugin paths |
| `cmd/micecam_ui/CameraSourceModel.h` | 48 | Source-grouped QAbstractListModel header |
| `cmd/micecam_ui/CameraSourceModel.cpp` | 82 | Model populated from PluginRegistryService sources |
| `tests/unit/test_plugin_registry.cpp` | 217 | 14 tests: bundled discovery, linked validation, enable/disable, diagnostics |
| `tests/unit/test_linked_plugin_config.cpp` | 114 | 9 tests: add/remove, save/load roundtrip, missing file handling |

### Updated files (8)

| File | Change |
|------|--------|
| `internal/domain/PluginRegistry.h` | Added `get_sources()` method declaration |
| `internal/domain/PluginRegistry.cpp` | Added `get_sources()` implementation returning `vector<PluginSource>` |
| `internal/infrastructure/CameraManager.h` | Added `PluginRegistryService*` member, `get_sources()`, `get_devices_for_source()` |
| `internal/infrastructure/CameraManager.cpp` | Added `set_plugin_registry()`, `get_sources()`, `get_devices_for_source()` |
| `cmd/micecam_ui/AppCameraModel.h` | Added `SourceIdRole`, `SourceGroupRole` to CameraRoles; added `sourceId`, `sourceGroup` to CameraRow |
| `cmd/micecam_ui/AppCameraModel.cpp` | Handle new roles in `data()`, `roleNames()`, `get()` |
| `cmd/micecam_ui/AppController.h` | Added `CameraSourceModel*` property, `PluginRegistryService` member |
| `cmd/micecam_ui/AppController.cpp` | Constructs PluginRegistryService, wires CameraManager, populates source model in `refreshCameras()` |
| `cmd/micecam_ui/main.cpp` | No substantive change (registry bootstrapped through AppController) |
| `cmd/micecam_ui/CMakeLists.txt` | Added `CameraSourceModel.cpp` to sources |
| `CMakeLists.txt` | Added `LinkedPluginConfig.cpp`, `PluginRegistryService.cpp` to `micecam_encoding`; added 2 new tests; added `CameraSourceModel.cpp` to test_app_models/app_controller targets |

## Design Decisions

1. **PluginRegistryService lives in AppController**: Rather than creating the registry in main.cpp and passing it down, AppController constructs and owns its own `PluginRegistryService`. This matches the existing pattern where AppController owns `CameraManager`. The registry is wired to CameraManager via `set_plugin_registry()`.

2. **Bundled plugins path**: Defaults to `"../3rdParty/bundled_plugins"` relative to the binary. This is taken as a constructor parameter so tests can override it.

3. **LinkedPluginConfig format**: Uses a simple JSON object `{"linked_plugins": ["/path1", "/path2"]}` stored at `{config_dir}/linked_plugins.json`, following the ConfigLoader pattern.

4. **Diagnostics**: PluginRegistryService records structured diagnostics (plugin_id, error_code, message) for both bundled and linked plugin failures. These get mapped to PluginSource diagnostics_state.

5. **CameraSourceModel**: Follows the same QAbstractListModel pattern as AppCameraModel, with `populateFromSources()` taking PluginSource and PluginDeviceInfo vectors.

6. **Backward compatibility**: `CameraManager::discover_all()` and `get_devices()` are unchanged. `CameraRow` gained new fields (`sourceId`, `sourceGroup`) that default to empty, so existing camera refresh code works identically.

## Verification Results

### cmake --build build -j 4
```
[100%] Built target micecam_ui
```

### ctest (new tests)
```
24/27 Test #24: test_plugin_registry ............. Passed    0.03 sec
25/27 Test #25: test_linked_plugin_config ........ Passed    0.02 sec
```

### ctest (full suite)
```
100% tests passed, 0 tests failed out of 27
Total Test time (real) = 19.20 sec
```

## Acceptance Checklist

- [x] Bundled plugins discovered from directory with `plugin.json`
- [x] Invalid manifest recorded with structured diagnostic
- [x] Missing directory handled gracefully (no crash)
- [x] `addLinkedDirectory()` validates manifest before accepting
- [x] `addLinkedDirectory()` rejects missing plugin.json
- [x] `addLinkedDirectory()` rejects invalid schema with structured error
- [x] `removeLinkedDirectory()` persists removal to config
- [x] `enablePlugin()`/`disablePlugin()` toggle pending restart flag
- [x] `getSources()` returns PluginSource with correct grouping
- [x] `getPlugins()` returns only enabled plugins
- [x] `CameraSourceModel` exposes source-grouped data via Qt roles
- [x] `AppCameraModel` has `SourceIdRole` and `SourceGroupRole`
- [x] LinkedPluginConfig add/remove round-trips through save/load
- [x] Existing backends (FFmpeg/Mock/OAK) remain operational
- [x] All 27 existing tests pass — zero regressions

## Remaining Risks

- The bundled plugins path `"../3rdParty/bundled_plugins"` is relative to CWD and will not work when the binary is installed elsewhere. This should be resolved in Phase 2+ with a proper install-time path resolution.
- `CameraManager::get_devices_for_source()` is a stub — actual device-to-source routing will need to be implemented when plugin processes launch and enumerate devices in Phase 2+.
