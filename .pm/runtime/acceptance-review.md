# Acceptance Review: Spike Iteration 1

## Task
`.pm/runtime/next-task.md` — Feasibility spike for OAK H264 + FFmpeg hardware encoder

## Verdict
**ACCEPTED**

## Evidence
- All 8 ACs met or skipped with documented reason
- Part B: VideoToolbox selected on macOS, produces valid H264 MP4 (ffprobe confirms)
- Part B: Fallback chain verified (invalid encoder → libx264 → valid MP4)
- Part A: Skipped — no OAK device, code guarded with `#ifdef HAS_DEPTHAI`
- Build: standalone cmake target, zero Qt dependency
- Git commit `e010134` — 5 files, clean scope

## Issues Found
- VideoToolbox B-frames cause PTS/DTS errors → fix: `max_b_frames=0`
- Color range warning (cosmetic) → fix: set `AVCOL_RANGE_MPEG`
- Both documented in spike report, not blocking

## Next Action
`delegate` — proceed to Stage 2 (Foundation): CMake + domain model + plugin interfaces
# Acceptance Review - UI Polish Round 1 Rework Required

## Verdict

Rejected: rework required.

## Evidence

- Independent build command passed after local dependency repair:
  - `cmake -B build -S . -DBUILD_UI=ON`
  - `cmake --build build --target micecam_ui -j`
- Build produced only pre-existing `MockCameraModel.cpp` signed/unsigned warnings.
- Runtime smoke showed app still tries to load remote mock preview images and receives 404s.
- Worker report was stale from a previous backend task and did not document this UI task.

## Issues

1. Sidebar was changed to a dark navy panel, but every provided UIDesign screen uses a light translucent/system sidebar. This violates the approved Apple HIG direction and `home.png`/`alerts.png`/`logging.png`.
2. Alerts page remains vertically stacked; controls are not right-aligned in the reference two-column settings-row structure.
3. Alerts defaults drift from spec/reference: watchdog should show `3` seconds, yellow threshold `0.1`, red threshold `1.0`.
4. Alerts uses emoji for eye/test action instead of project icons or `AppIcon`, causing cross-platform inconsistency.
5. Logging page uses a dark terminal-style log preview, but `logging.png` shows a light bordered monospaced preview.
6. Worker attempted `rm -rf build`, which is forbidden going forward. Configure/build must run without deleting the build directory.
7. `.pm/runtime/worker-report.md` was not updated for this task.

## Next Action

Request rework with explicit UI fidelity fixes and fresh report.

---

# Acceptance Review - UI Polish Round 1 Second Rework Required

## Verdict

Rejected again: actual screenshot shows the page is not acceptable.

## Evidence

User screenshot `截屏2026-05-14 21.33.54.png` shows:
- App appears blurry / low-resolution on Retina.
- Drop-rate sliders collapsed into tiny handles instead of full tracks.
- Watchdog stepper renders as a black block.
- Desktop/Sound switches render as black Basic controls.
- Alerts row layout still does not match `UIDesign/alerts.png`.

## Required Fix

Second rework task written to `.pm/runtime/next-task.md`: remove whole-window raster layer, replace naked `SpinBox`/`Switch` with custom HIG controls, and give slider controls explicit stable widths.

## Second Rework Result

Accepted for Round 1 scope.

Evidence:
- `cmake -B build -S . -DBUILD_UI=ON` passed.
- `cmake --build build --target micecam_ui -j` passed.
- Runtime launch no longer reports Alerts/Logging QML errors.
- `/tmp/micecam_alerts_probe.png` shows full slider tracks, a light custom stepper, custom switches, and correct `0.1`/`1.0` values.

Follow-up:
- Home grid visual review is still blocked by remote mock image 404.
- Font fallback and titlebar DragHandler warnings need follow-up.

---

# Acceptance Review - UI Polish Round 2

## Verdict

Accepted with follow-up.

## Evidence

- `cmake -B build -S . -DBUILD_UI=ON` passed.
- `cmake --build build --target micecam_ui -j` passed.
- Runtime no longer logs `CameraCard.qml` remote image 404s.
- Runtime no longer logs `startSystemMove` null/type warnings during normal launch.
- Screenshot `/tmp/micecam_home_after.png` shows local non-broken camera cards with overlays, REC badges, warning state, and stable grid spacing.

## Remaining Issues

- Camera previews are abstract grid placeholders, not close enough to the reference mouse-camera visual.
- Status bar text is missing/clipped in the screenshot; only icons are clearly visible.
- Global font warning remains because many QML files still hardcode `SF Pro Text`.

## Next Action

Delegate Round 3: global font sweep, status bar readability, and local realistic preview assets/card fidelity.

---

# Delegation Review - UI Polish Round 3 Sync Attempt

## Verdict

Blocked: sync OpenCode Intern attempt did not produce a fresh report.

## Evidence

- Command started: `opencode run "/harness-intern Read and execute .pm/runtime/next-task.md exactly. Write .pm/runtime/worker-report.md and create one git commit for your task changes only." --file .pm/runtime/next-task.md`
- The worker read the task and relevant QML files.
- The only observed file modification was a partial `AppStatusBar.qml` font-family replacement.
- `.pm/runtime/worker-report.md` remained stale from Round 2.
- The process was interrupted after more than 10 minutes without new output or report refresh.

## Next Action

Retry Round 3 delegation from the current dirty worktree. The task packet has been annotated so the next worker continues the partial edit and avoids long-running image-analysis tooling.

---

# Acceptance Review - UI Polish Round 3

## Verdict

Rejected: rework required.

## Evidence

- Worker commit: `3cbfad3`.
- Independent build passed: `cmake --build build --target micecam_ui -j`.
- Independent runtime log was empty: no font warning and no remote image 404.
- Independent screenshot: `/tmp/micecam_pm_home_round3.png`.

## Issues

1. Bottom status bar still does not meet acceptance criteria. The screenshot shows icons only; metric text such as `00:42:17`, `5 cameras`, `76,230 frames`, `29.97 fps avg`, `3.2 GB`, and `45% disk remaining` is not visible.
2. User feedback: right side is still too full. The camera grid/right edge needs visible breathing room.
3. Worker report says `Commit Hash: To be committed` even though commit `3cbfad3` was created. Report evidence is inconsistent.

## Next Action

Request rework with explicit fixes for `AppStatusBar.qml` text binding and `CameraGridView.qml` right margin behavior.

---

# Acceptance Review - UI Polish Round 3 Rework

## Verdict

Accepted.

## Evidence

- Worker commit: `f1308d7`.
- Independent build passed: `cmake --build build --target micecam_ui -j`.
- Independent runtime log: `.pm/runtime/micecam_round3_rework_runtime.log` is empty.
- Independent screenshot: `.pm/runtime/micecam_home_after_round3_rework.png`.
- Screenshot confirms bottom status bar shows icon + text for all six metrics.
- Screenshot confirms the camera grid has visible right-side breathing room and no longer feels flush.
- Font scan passed: no `SF Pro`, `SF Mono`, or `.AppleSystemUIFont*` hardcoded aliases remain in `cmd/micecam_ui/qml`.

## Issues

None blocking for Round 3 rework.

## Next Action

Delegate Round 4: notification popover, right-click camera context menu, fullscreen/enlarged camera overlay, and preflight failure modal.

---

# Acceptance Review - UI Polish Round 4

## Verdict

Rejected: rework required.

## Evidence

- Worker commit: `1dc8e49`.
- Build passed and runtime log was clean in worker evidence.
- User manually exercised the UI and provided screenshots copied to `.pm/runtime/micecam_user_round4_*.png`.

## Issues

1. Right-click camera context menu is not usable.
2. Notification dropdown/popover clips or overflows text; `.pm/runtime/micecam_user_round4_notification.png` shows long titles running out of the panel.
3. Logging page is visually broken; `.pm/runtime/micecam_user_round4_logging.png` shows the recent log preview collapsed into a narrow vertical strip.
4. Fullscreen close button does not dismiss the fullscreen overlay.
5. Top-right Settings button does not navigate/open settings; `.pm/runtime/micecam_user_round4_settings.png` remains on the home grid.
6. Record/Stop button has a small visual defect; `.pm/runtime/micecam_user_round4_record_button_border.png` shows a stray red line at the right edge of the red Stop segment.

## Next Action

Request narrow Round 4 rework for these interaction/layout failures, including the Stop button border defect.

---

# Acceptance Review - UI Polish Round 4 Rework

## Verdict

Rejected: rework still failed.

## Evidence

- Worker commit: `710bd5d`.
- User screenshot copied to `.pm/runtime/micecam_user_round4_rework2_logging_still_bad.png`.
- User feedback: “logger 依旧很垃圾。右键依旧无效。其他的测试我都没做”

## Issues

1. Logging page remains visually broken. The recent log preview is still a narrow vertical strip, not a readable panel.
2. Right-click remains ineffective for the user.
3. Stop/timer control still has red border artifacts after the attempted fix.

## Next Action

Request Round 4 Rework 2. This time the task forbids micro-tweaking the current broken Logging row-slot layout and requires a custom QML context-menu overlay instead of Qt `Menu`/`TapHandler`.

---

# Decision Gate - Sidebar Camera List Behavior

## Status

Needs user decision before more implementation.

## Evidence

The user noted the camera list in the sidebar is currently functionally useless and suggested clicking should enter fullscreen or a camera information page. This changes the intended navigation model for the main UI and affects:
- Sidebar camera row click behavior.
- Camera detail/fullscreen surfaces.
- Whether right-click is primary or secondary interaction.
- Whether `UIDesign/history.png` should become a first-class sidebar/settings item in the same phase.

## Decision Needed

Choose the camera list click behavior:
- A: single-click opens fullscreen/enlarged camera view.
- B: single-click opens a camera detail/info page; the page includes a fullscreen action.
- C: single-click focuses/selects the camera card in the grid; double-click opens fullscreen.

## Next Action

Pause delegation until this behavior is decided.

---

# Decision Resolution - Sidebar Camera List Behavior

## Decision

Accepted user decision: clicking a sidebar camera opens that camera's detail/configuration page. Right-click remains a shortcut path for Configure and Fullscreen.

## Implications

- Sidebar camera list becomes primary navigation into camera detail/config.
- Right-click Configure must open the same detail/config page.
- Fullscreen remains available from camera detail and right-click menu.
- History page from `UIDesign/history.png` remains a follow-up surface; it is not part of the immediate camera-detail task unless implementation scope allows.

## Next Action

Delegate Round 5 task.

---

# Acceptance Review - UI Polish Round 5 Camera Detail

## Verdict

Partially accepted, rework required.

## Evidence

- Worker commit: `ec0eff9`.
- User screenshot copied to `.pm/runtime/micecam_user_round6_camera_detail_needs_resolution_fps.png`.
- The camera detail/config page now opens from sidebar selection and shows a large preview, metrics, and a few controls.

## Issues

1. The detail page treats `Resolution` and `Frame Rate` as read-only metrics, but cameras have selectable acquisition modes. Users must be able to configure these values per camera.
2. The metrics/config boundary is cramped in the screenshot, with rows visually colliding around the current read-only fields and controls.
3. Right-click Configure and sidebar click must keep landing on the same usable camera detail/config page after this rework.

## Next Action

Delegate Round 6: add selectable resolution, frame-rate, and stream/pixel format controls to the camera detail/config page, keeping the current UI-state mock boundary and preserving prior fixes.

---

# Acceptance Review - UI Polish Round 6 Dirty Attempt

## Verdict

Rejected: rework required before any commit.

## Evidence

- User screenshot copied to `.pm/runtime/micecam_user_round6_rework_ugly_no_home.png`.
- User feedback: "这个很丑陋" and "现在没有 home 页面的icon，一旦跳转就回不去了".

## Issues

1. The camera detail page added acquisition controls but collapsed into overlapping, thin horizontal rows. This fails basic UI hierarchy.
2. Section headings render like stray bordered bars, making the page look broken rather than configurable.
3. Controls are crammed to the right and collide with metrics/content.
4. The sidebar has no persistent Home/Cameras navigation row with an icon, so the user lacks a clear way back to the camera grid after opening detail.

## Next Action

Delegate Round 6 rework: keep resolution/frame-rate/stream controls, redesign the page hierarchy, and add a persistent Home/Cameras sidebar row.

---

# Acceptance Review - UI Polish Round 6 Rework

## Verdict

Rejected: rework 2 required.

## Evidence

- Worker commit: `69f296b`.
- Worker report: `.pm/runtime/worker-report.md`.
- User screenshot: `.pm/runtime/micecam_user_round6_rework_ugly_no_home.png`.

## Issues

1. The sidebar Home/Cameras row was required but not implemented. `AppSidebar.qml` was not changed by the worker.
2. The detail page still reads as a cramped/collapsed form in the user's screenshot.
3. The worker report contains the wrong commit hash: it says `b8586b4`, but actual HEAD is `69f296b`.
4. The required implementation report `docs/reports/implements/phase-ui-polish-05-15-05.md` was not created.
5. The worker marked unimplemented or unverified criteria as accepted, which invalidates the report as an acceptance artifact.

## Next Action

Delegate Round 6 Rework 2 with a narrower task: add persistent sidebar Home/Cameras navigation, repair detail page layout, preserve acquisition controls, create the implementation report, and write a correct worker report.

---

# Acceptance Review - UI Polish Round 6 Rework 2 Dirty State

## Verdict

Rejected before commit: design rework required.

## Evidence

- User reviewed the Acquisition Configuration section and rejected segmented buttons.
- User feedback: real cameras expose different supported resolutions and frame rates, so the UI needs dropdowns that can be populated from enumerated camera capability data.

## Issues

1. Segmented buttons are the wrong control for real capability enumeration. They assume a small fixed option set and do not map cleanly to backend-provided capabilities.
2. The UI needs dropdown/ComboBox controls for `Resolution`, `Frame rate`, and `Pixel/Stream mode`.
3. Option data should be structured with `label` and `value` roles or equivalent, so it can later be replaced by device capability models without redesigning the UI.

## Next Action

Delegate Round 6 Rework 3: replace segmented acquisition controls with dropdown/ComboBox controls backed by enum/capability-friendly option models, while preserving the sidebar Home/Cameras fix and clean detail layout.

---

# Product Review - UI Polish Final Known Issue

## Verdict

Proceed with one final polish fix, then stop implementation and run overall evaluation.

## Evidence

- User reports the UI is now broadly complete enough.
- Remaining small visual issue: camera card corner radii are inconsistent. Warning card border is rounded, but bottom overlay/inner content creates hard square corners.

## Decision

Do not add more UI surfaces in this stage. Fix only the camera-card corner mismatch, preserve dropdown/capability-friendly acquisition configuration, then stop for:
- Overall UI completeness review.
- Wiring-up readiness check for real camera capability enumeration and backend integration.
- Follow-up backlog classification.

## Next Action

Delegate final corner polish task.

---

# Acceptance Review - Final Corner Polish

## Verdict

Rejected: issue not solved.

## Evidence

- User stated: "问题是他的问题还没解决啊".
- PM review found the final fix in `9d284bd` assumes `Rectangle { radius; clip: true }` is enough to rounded-mask child content. That is not a reliable fix for Qt Quick child geometry and matches the unresolved visual complaint.

## Issue

`CameraCard.qml` still needs explicit layer geometry:
- Preview/content should carry the top rounded shape.
- Bottom overlay should carry the bottom rounded shape.
- Bottom overlay top edge should remain straight.
- Warning border should trace the same radius.

## Next Action

Delegate a narrowly scoped rework on top of `9d284bd`. Do not proceed to overall evaluation until this card-corner issue is actually fixed.

---

# Acceptance Review - Card Corner Rework

## Verdict

Accepted for code, with report caveat.

## Evidence

- Rework commit: `07da9d9`.
- PM independent build passed: `cmake --build build --target micecam_ui -j`.
- PM independent runtime log empty: `.pm/runtime/micecam_pm_card_corner_eval_runtime.log`.
- `CameraCard.qml` now uses explicit shared layer geometry:
  - `cardRadius`.
  - `previewSurface` has `radius` and `clip`.
  - `bottomBar` has `radius`, `clip`, and a top same-color patch to keep the top edge straight.
  - Warning border uses the same `cardRadius`.

## Caveat

The worker report still lists stale commit hash `cf44feb`; actual final commit is `07da9d9`. Treat the report hash as unreliable. The code and independent verification are accepted.

## Next Action

Stop implementation and perform overall UI completeness plus wiring-up readiness evaluation.

---

# Acceptance Review - Canvas Card Corner Fix

## Verdict

Accepted.

## Evidence

- Rework commit: `3be4133`.
- PM independent build passed: `cmake --build build --target micecam_ui -j`.
- PM independent runtime log empty: `.pm/runtime/micecam_pm_canvas_corner_runtime.log`.
- Code inspection confirms `CameraCard.qml` now clips actual Canvas preview painting:
  - `roundedRectPath(...)` helper exists in `previewCanvas`.
  - `ctx.clip()` is called before noise/grid/vignette painting.
  - `bottomBarCorners` Canvas clips/fills bottom-only rounded corners.
  - Warning border uses shared `cardRadius`.

## Notes

Intern again used forbidden image-analysis tooling during verification, so PM acceptance is based on code inspection, independent build/runtime, and user-provided screenshot feedback rather than the worker's image-analysis claim.

## Next Action

Stop implementation. UI stage is ready for overall completeness and wiring-up readiness evaluation.
