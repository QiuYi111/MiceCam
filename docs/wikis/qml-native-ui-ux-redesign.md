# MiceCam QML Native UI/UX Audit and Redesign

## 1. Context and Scope

This document evaluates the current native QML interface under `cmd/micecam_ui/qml/main.qml` and proposes a QML-native redesign direction for MiceCam.

The assessment is based on:
- Current QML implementation in `cmd/micecam_ui/qml/main.qml`
- UI-facing backend contract in `cmd/micecam_ui/PipelineController.h` and `.cpp`
- Legacy Python UI in `cmd/gui/gui/main_window.py`

Important limitation:
- The repository currently does not contain formal PRD files under `docs/requirements/` or phase plans under `docs/plan/`.
- Therefore, product intent below is inferred from the existing capture workflow and from the migration note that the product has recently moved from Python app to QML.

## 2. Executive Assessment

The current QML UI is functionally usable but visually and structurally it is still a direct port of a tooling dashboard. It is not yet a native desktop product experience.

Main diagnosis:
- The interface is form-first instead of task-first.
- The visual hierarchy is flat, so preview, session identity, system status, and risk signals compete equally.
- The design language is closer to developer utilities than to a polished operator-facing application.
- The current single-file QML scene is hard to evolve into a refined system because layout, style, state mapping, and interaction logic are all coupled.

As a result, the product currently feels:
- practical
- dense
- technically honest
- but not calm, premium, or trustworthy enough for a long-running capture workflow

If the target is an Apple-like minimal and precise experience, the redesign should not mean "white glass and blur everywhere". It should mean:
- fewer decisions per screen
- clearer hierarchy
- restrained motion
- large areas of visual calm
- high-quality spacing and typography
- explicit system state transitions
- details that feel carefully tuned rather than decorated

## 3. Current UI Audit

## 3.1 Structural issues

Current screen structure:
- Header
- Source & Output
- Statistics
- Controls
- Live Preview
- System Log

Problems:
- The most important object on this product is the recording session, but the UI is not organized around a session lifecycle.
- Preview is visually below configuration and controls, even though preview is the user's main confidence signal.
- Logs take a large persistent vertical block even when they are not actionable.
- Statistics are shown as isolated numbers without semantic grouping into health, performance, and storage outcomes.
- Everything sits in one long scroll view, which creates a "settings page" feel instead of an operational workspace.

## 3.2 Interaction issues

Current interaction model:
- User configures several fields.
- User presses start.
- Some fields disable.
- Stats update.
- Stop triggers optional decode.

Problems:
- No strong preflight confirmation before recording begins.
- No distinct state transition between idle, ready, recording, stopping, decoding, and completed.
- Decode progress is only a thin inline progress bar, so post-recording flow feels under-explained.
- Error handling is backend-oriented. `errorOccurred` exists, but the current QML does not establish a first-class visual error pattern.
- Start action assumes resolution parsing and camera model access succeed without validating empty or malformed states.

## 3.3 Visual issues

Current design traits:
- Strong blue header bar
- Emoji labels in production UI
- GroupBox-heavy composition
- Conventional controls and stat cards
- Dark log panel inside otherwise light UI

Why it misses the target style:
- The saturated header dominates too much attention.
- Emoji-based section markers reduce product seriousness.
- GroupBox chrome adds borders everywhere, which fragments the canvas.
- The screen is built from boxes rather than from rhythm, spacing, and typographic hierarchy.
- The page has almost no intentional negative space.

This is not an Apple-like minimal interface. It is a capable utility panel.

## 3.4 QML architecture issues

Current QML implementation is concentrated in one file. That creates several problems:
- Design tokens are implicit rather than centralized.
- Reusable components do not exist yet.
- State-specific styling is repeated inline.
- Layout refactors will become fragile.
- It is difficult to enforce consistency once more screens or dialogs are added.

For a native QML product, this is the main architectural gap. Good QML UI quality depends heavily on componentization and state-driven composition.

## 4. Product Intent Reframed as User Tasks

Even without a PRD, the current product strongly suggests four primary user goals:

1. Configure a reliable capture session quickly.
2. Confirm the camera feed and system health before and during recording.
3. Avoid silent failures such as dropped frames, wrong output path, or decode confusion.
4. Finish recording and understand what happened without scanning logs.

This means the interface should optimize for:
- operational confidence
- low cognitive load
- immediate health visibility
- graceful transitions between lifecycle stages

Not for:
- exposing every internal metric equally
- permanently showing engineering logs
- maximizing control density on one screen

## 5. Recommended UX Direction

## 5.1 Core concept

Redesign the app as a focused "session workspace" instead of a scrollable settings dashboard.

Primary layout:
- Left or top: session identity and setup
- Center: large live preview stage
- Right or bottom: health summary, capture stats, and contextual events
- Persistent bottom or side action area: start, stop, decode, export, reveal in Finder

The UI should feel like one object with several layers of focus, not six stacked widgets.

## 5.2 State model

The new UI should have explicit visual states:
- Idle
- Ready
- Recording
- Stopping
- Decoding
- Completed
- Error

Each state should modify:
- title/subtitle copy
- main action label
- accent color usage
- visibility of secondary controls
- event messaging

Example:
- Idle: preview stage is calm, setup fields editable, health shows availability checks
- Recording: preview dominates, timer and data rate become primary, setup collapses to summary
- Decoding: recording controls disappear, progress gets dedicated attention, next-step actions appear

## 5.3 Information hierarchy

Priority order should be:
1. Session state
2. Live preview
3. Primary action
4. Health indicators
5. Essential session metadata
6. Detailed stats
7. Logs and diagnostics

This is different from the current page, where configuration fields and logs consume too much persistent emphasis.

## 5.4 Error and trust design

This product controls long-running capture jobs, so trust matters more than ornament.

Recommended error model:
- Inline validation for user-editable fields
- Banner-level alerts for recoverable issues
- Modal confirmation only for destructive or irreversible actions
- A compact event timeline for warnings, start/stop events, and decode results

Do not hide problems in the log stream.
The user should understand within one glance:
- whether capture is healthy
- whether storage target is valid
- whether frames are being dropped
- whether decode succeeded

## 6. Apple-like Visual Direction, Without Imitation

The target style should be interpreted as:
- precise
- warm-neutral
- restrained
- soft but not blurry
- elegant through proportion

It should avoid:
- neon accents
- sci-fi gradients
- pseudo-AI holographic visuals
- excessive translucency
- heavy shadows

## 6.1 Visual principles

Use:
- warm neutral background rather than pure blue branding blocks
- a single restrained accent color
- large radii but not bubble UI
- thin separators instead of repeated boxed borders
- high-quality typography scale
- subtle elevation only where interaction needs it

Recommended tone:
- background: off-white / light stone
- surfaces: white with slight warmth
- text: charcoal and muted gray
- accent: desaturated graphite-blue or quiet blue-gray
- caution: muted amber
- error: restrained crimson
- success: deep natural green

## 6.2 Typography

The current interface relies on default control styling and bold blue numerics.
The redesign should use typography as the main hierarchy driver.

Recommendation:
- Use SF Pro Text / SF Pro Display where available on macOS, with clean fallback stacks for other platforms.
- Prefer weight and spacing over large color shifts.
- Use three dominant scales:
  - window title / state title
  - section subtitle / session summary
  - compact labels / metadata

Stat numerals should feel calm and stable, not promotional.

## 6.3 Motion

QML is strong at state transitions. Use that, but with restraint.

Recommended motion:
- soft opacity + y-offset reveal for state cards
- animated width/opacity transitions when setup collapses during recording
- progress transitions that ease smoothly rather than jump
- preview placeholder to live-feed transition with short fade

Avoid:
- springy bounce
- glowing pulses
- exaggerated shimmer

## 7. Proposed Screen Architecture

## 7.1 Main window layout

Recommended desktop layout:

- Top bar
  - app name
  - session status chip
  - current session title
  - quick actions

- Main content split
  - Primary stage
    - large live preview
    - preview placeholder / no camera state
    - recording overlay with elapsed time and live health
  - Secondary rail
    - setup card or session summary
    - health card
    - metrics card
    - activity card

- Bottom action dock
  - start/stop primary button
  - decode toggle or post-process status
  - output destination summary

This removes the long scroll and turns the app into a stable workspace.

## 7.2 On smaller screens

If width is constrained:
- Preview stays first.
- Secondary rail moves below preview.
- Logs become a collapsible sheet.
- Less-important metrics condense into two rows.

QML should support this via breakpoints or width-derived layout switching, not by squeezing desktop proportions.

## 7.3 Suggested content modules

1. SessionHeader
- current state
- session name
- destination summary

2. PreviewStage
- live image
- idle placeholder
- recording overlay
- decode overlay when active

3. SetupPanel
- camera
- resolution
- fps
- output directory
- auto-decode

4. HealthPanel
- camera connected
- write path available
- dropped frame status
- decode readiness

5. MetricsPanel
- FPS
- captured frames
- dropped frames
- throughput
- format

6. ActivityPanel
- compact recent events
- expandable diagnostics

## 8. QML-Native Component Strategy

The redesign should not remain in one `main.qml`.
It should be decomposed into a small design system and scene-level composition.

Suggested structure:

- `cmd/micecam_ui/qml/App.qml`
- `cmd/micecam_ui/qml/theme/Theme.qml`
- `cmd/micecam_ui/qml/theme/Tokens.js` or singleton token object
- `cmd/micecam_ui/qml/components/WindowHeader.qml`
- `cmd/micecam_ui/qml/components/PreviewStage.qml`
- `cmd/micecam_ui/qml/components/SetupPanel.qml`
- `cmd/micecam_ui/qml/components/MetricTile.qml`
- `cmd/micecam_ui/qml/components/HealthBadge.qml`
- `cmd/micecam_ui/qml/components/EventList.qml`
- `cmd/micecam_ui/qml/components/PrimaryButton.qml`

Design benefits:
- centralized color and spacing tokens
- reusable interaction states
- easier iteration on visual language
- clearer ownership between layout and behavior

## 9. Backend Contract Gaps Blocking Better UX

The current `PipelineController` is enough for a functional MVP but not enough for a refined UX.

To support the proposed UI cleanly, add or expose:

1. Explicit session state enum
- Instead of deriving everything from `isRecording` and `decodeProgress`
- Example states: `idle`, `recording`, `decoding`, `completed`, `error`

2. Validation and readiness properties
- `hasAvailableCamera`
- `outputPathValid`
- `canStartRecording`
- `lastErrorMessage`

3. User-facing session metadata
- elapsed recording time
- resolved output path
- decoded output path
- selected camera name

4. Event model instead of raw string log list
- severity
- timestamp
- category
- message

5. Health summary properties
- storage writable
- camera online
- dropped frame warning state
- decode active

Without these properties, QML will keep reconstructing product state indirectly, which leads to brittle UI logic.

## 10. Detailed Design Recommendation

## 10.1 Layout behavior

Idle / Ready:
- show setup in full
- preview shows calm placeholder or passive feed
- primary CTA reads `Start Recording`

Recording:
- collapse setup into summary rows
- enlarge preview
- pin live metrics near preview, not far away in another section
- change CTA to `Stop Recording`

Decoding:
- preserve session summary
- replace setup with decode progress and output actions
- keep preview visible only if it helps context; otherwise use completion summary

Completed:
- highlight saved location and decode result
- show `Open Folder` and `Start New Session`

## 10.2 Preview stage

The preview should become the visual anchor of the app.

Recommendations:
- generous aspect-ratio frame with rounded corners
- subtle border and inner shadow only if needed for separation
- overlay chip for recording state
- overlay metrics limited to 2-3 essential live indicators
- placeholder illustration should be abstract and understated, not cute or futuristic

## 10.3 Metrics

Metrics should be split by meaning:

Health:
- dropped frames
- camera connection
- disk path status

Performance:
- FPS
- throughput
- format

Outcome:
- captured frames
- session duration
- decoded export status

This is much more useful than a flat stat grid.

## 10.4 Logs and diagnostics

System logs should not be a permanently dominant panel.

Recommended behavior:
- show a compact "Recent Activity" feed with 4-6 items
- include severity tint and timestamp
- provide a secondary action to expand full logs when needed

This keeps engineering visibility without sacrificing calmness.

## 11. Styling Tokens Recommendation

Example direction:

- Background: `#f5f3ef`
- Surface: `#fcfbf8`
- SurfaceElevated: `#ffffff`
- Border: `#ddd7cf`
- Separator: `#e8e2da`
- TextPrimary: `#1f1f1c`
- TextSecondary: `#6b6a65`
- Accent: `#5d7484`
- AccentStrong: `#445865`
- Success: `#55705c`
- Warning: `#9b7a45`
- Error: `#9a4b47`

Spacing scale:
- 6, 10, 14, 20, 28, 40

Corner radii:
- 10 for controls
- 16 for cards
- 22 for preview stage

Shadows:
- one soft shadow family only
- low opacity
- short blur

## 12. Phased Implementation Plan

## Phase A: Foundation
- Extract theme tokens and reusable button/card components.
- Split `main.qml` into scene plus reusable primitives.
- Remove emoji and heavy GroupBox usage.

## Phase B: State-driven workspace
- Rebuild layout into preview stage + side rail + action dock.
- Introduce explicit session-state presentation in QML.
- Convert logs into recent activity feed.

## Phase C: Backend contract upgrades
- Add structured UI state properties to `PipelineController`.
- Add user-facing validation and health properties.
- Add richer event stream model.

## Phase D: Refinement
- Tune spacing, typography, transitions, and responsive breakpoints.
- Add empty, error, decode, and completed states.
- Perform visual QA on macOS and non-macOS builds.

## 13. High-Priority Recommendations

If only a few things are done next, do these first:

1. Replace the single long scroll form with a two-zone workspace centered on preview.
2. Introduce an explicit session-state model instead of relying on `isRecording` plus ad hoc conditions.
3. Move logs into a compact activity panel and elevate health status into first-class UI.
4. Establish a QML theme token system before further styling work.
5. Remove visual noise: emoji, saturated header bar, repeated borders, dense GroupBox framing.

## 14. Final Verdict

The current QML interface proves the native pipeline is operational, but it does not yet express the quality of the underlying system. It still feels like a technical control panel.

The right redesign is not "more decoration". It is a deeper reorganization around session workflow, visual calm, trust signals, and QML-native component architecture.

If executed well, MiceCam can move from:
- "a useful internal recording tool"

to:
- "a polished native capture workstation with quiet confidence"

That is the correct path for an Apple-like minimal product language.

## 15. Migration Gap Analysis: Python App to QML

The migration from the Python app to QML preserved the main functional blocks, but it also preserved most of the old information architecture. The result is that the technology stack changed while the product experience barely changed.

Observed migration pattern:
- The QML scene mirrors the old scroll-based grouping from the Python app.
- The core workflow is still "fill a form, start, read stats, inspect log".
- Backend state got richer in `PipelineController`, but the QML scene is not yet using that richer state to reshape the interface.

What improved with QML:
- Preview can now be integrated natively through `QQuickImageProvider`.
- UI can move toward declarative states, transitions, and adaptive layouts.
- The backend already exposes session lifecycle data that the Python UI did not model as cleanly.

What did not improve enough:
- Layout remains vertical, dense, and utilitarian.
- Visual language still looks like a developer tool.
- Interaction still feels field-centric rather than workflow-centric.
- QML strengths such as states, component composition, and responsive layout switching are mostly unused.

The design conclusion is important:
- this is not a "styling pass" problem
- it is a product-structure problem

## 16. UX Heuristics Review

The current scene can be evaluated against common desktop UX heuristics:

### 16.1 Visibility of system status

Partially successful.

Strengths:
- live stats update during recording
- recording enable/disable states are visible
- preview clearly indicates when it is offline

Weaknesses:
- no strong status region at the top level
- decode state is too easy to miss
- warnings such as dropped frames do not escalate visually

### 16.2 Match between system and user mental model

Weak.

Users think in sessions:
- choose source
- confirm preview
- record
- finish
- export or review

The current layout thinks in widgets:
- source box
- stats box
- controls box
- preview box
- log box

That mismatch increases cognitive load.

### 16.3 Error prevention

Underdeveloped.

Current gaps:
- no visible readiness checklist before start
- output directory validity is not surfaced
- malformed FPS input is not guarded in the UI
- camera availability and selected resolution compatibility are inferred rather than confirmed

### 16.4 Recognition over recall

Mixed.

The UI shows live numbers, but it hides contextual meaning:
- Is dropped frames at `3` acceptable or dangerous?
- Is the selected destination writable?
- Is decode still running or already failed?

The new design should turn raw telemetry into interpreted status.

### 16.5 Aesthetic and minimalist design

Currently below target.

Minimalism here should mean:
- fewer simultaneous blocks
- calmer use of accent color
- stronger type and spacing discipline
- less persistent chrome

The present interface is sparse in ornament but not truly minimal, because it still shows too many equal-priority regions at once.

## 17. Proposed Experience Model

The redesigned app should behave like a capture appliance, not a form editor.

### 17.1 Primary user story

Given a user wants to record a session,
when they open the app,
then they should immediately understand:
- whether the system is ready
- which camera is selected
- where data will be stored
- what to do next

Given a recording is active,
when the user glances at the screen,
then they should immediately understand:
- whether capture is healthy
- how long it has been running
- whether the preview looks correct
- whether intervention is needed

Given a session has ended,
when decoding is running or complete,
then the app should shift from "capture control" to "result handling" without making the user parse logs.

### 17.2 Product posture

The product should feel:
- deliberate
- dependable
- quiet
- technical without looking engineering-first

The product should not feel:
- playful
- gadget-like
- futuristic
- dashboard-heavy

## 18. New Screen Blueprint

The new interface should be built around one stable desktop workspace.

### 18.1 Desktop layout

Recommended 3-layer composition:

1. Window header
- app identity
- current state chip
- session title
- one utility action such as `Reveal Output`

2. Main workspace
- large preview stage on the left
- contextual rail on the right

3. Action dock
- persistent bottom action zone with one dominant CTA and two secondary summaries

ASCII wireframe:

```text
+----------------------------------------------------------------------------------+
| MiceCam                  [Ready]            Session 20260311_0830   Reveal Output |
+----------------------------------------------------------------------------------+
|                                                                 |                |
|  Preview Stage                                                   |  Session Setup |
|  - idle placeholder / live feed                                  |  Camera        |
|  - recording overlay                                              |  Resolution    |
|  - elapsed time + health chips                                    |  FPS           |
|                                                                 |  Output Path   |
|                                                                 |  Auto Decode   |
|-----------------------------------------------------------------|----------------|
|                                                                 |  Capture Health |
|                                                                 |  Camera OK      |
|                                                                 |  Storage OK     |
|                                                                 |  Drop Status    |
|                                                                 |----------------|
|                                                                 |  Recent Activity|
+----------------------------------------------------------------------------------+
| Output: recordings / mouse_A      Auto Decode: On      [ Start Recording ]       |
+----------------------------------------------------------------------------------+
```

### 18.2 Recording layout behavior

When recording starts:
- setup panel collapses into a concise session summary
- preview stage grows in emphasis
- health panel and live metrics remain visible
- action dock switches to `Stop Recording`

The right rail should stop behaving like a settings form and start behaving like a control/status rail.

### 18.3 Decoding layout behavior

When decoding starts:
- preview becomes secondary context or fades to a static completion still
- right rail foregrounds decode progress and output destination
- CTA changes from control action to result action such as `Open Export Folder`

This creates a clean lifecycle handoff instead of leaving the user in the same layout with a thin progress bar.

## 19. Responsive Behavior

QML should not merely scale controls; it should switch layout strategy.

### 19.1 Wide desktop

Threshold:
- about `>= 1180px`

Behavior:
- two-column workspace
- preview dominates horizontally
- action dock stays horizontal

### 19.2 Medium width

Threshold:
- about `880px` to `1179px`

Behavior:
- preview remains first
- right rail stacks below preview
- action dock may wrap into two rows

### 19.3 Compact width

Threshold:
- `< 880px`

Behavior:
- no always-visible multi-card rail
- show preview, then session card, then health, then activity
- advanced logs collapse behind a disclosure row

This matters because the current 600x900 window size already biases the layout toward vertical stacking. A redesign that only targets large desktop widths will regress on the current footprint.

## 20. Visual System Specification

### 20.1 Overall tone

The target is "Apple-like" in discipline, not in imitation.

Interpretation:
- soft warm neutrals
- restrained blue-gray accent
- exact spacing
- low visual noise
- polished control surfaces

Avoid:
- frosted-glass theater
- bright cobalt branding blocks
- heavy gradients
- glossy chrome
- sci-fi overlays

### 20.2 Color roles

Recommended palette:

- AppBackground: `#f3f0ea`
- Surface: `#fbf8f3`
- SurfaceRaised: `#ffffff`
- SurfaceMuted: `#f0ece6`
- BorderSubtle: `#ddd6cc`
- Separator: `#e6dfd5`
- TextPrimary: `#1d1d1b`
- TextSecondary: `#67655f`
- TextTertiary: `#8a867f`
- Accent: `#617786`
- AccentPressed: `#4b6170`
- Success: `#566f5c`
- Warning: `#9a7a47`
- Error: `#954b46`
- OverlayScrim: `#221f1a22`

Usage rules:
- accent color should appear mainly on the primary CTA, focused fields, progress, and active chips
- error color should be sparse and reserved for actual risk
- health-success green should be muted, not saturated

### 20.3 Typography

Recommended stack:
- macOS: `SF Pro Display`, `SF Pro Text`
- fallback: `PingFang SC`, `Helvetica Neue`, `Noto Sans`, `sans-serif`

Suggested scale:
- Display title: 28 / semibold
- State title: 22 / semibold
- Section title: 15 / medium
- Body: 13 to 14 / regular
- Meta label: 11 to 12 / medium
- Numeric stat: 26 to 32 / semibold with tabular figures where possible

Typography rules:
- rely on weight and contrast before relying on more color
- keep letter spacing neutral
- use sentence case instead of all caps

### 20.4 Surface and edge treatment

Recommended:
- preview frame radius: `24`
- card radius: `18`
- control radius: `12`
- separator lines over bordered boxes whenever possible

Shadow philosophy:
- one shadow family only
- very soft
- only for preview stage, action dock, and active floating surfaces

### 20.5 Motion specification

Recommended transitions:
- `180ms` opacity and position fade for card state swaps
- `220ms` size interpolation when panels expand or collapse
- `140ms` press state easing on buttons
- `240ms` progress value smoothing

Recommended easing:
- `Easing.OutCubic` or `Easing.InOutCubic`

Avoid:
- overshoot
- elastic motion
- animated glows

## 21. Detailed Interaction Design

### 21.1 Preflight before start

Before the primary CTA becomes enabled, the UI should validate:
- at least one camera is available
- a camera is selected
- output path is non-empty
- session name is non-empty
- FPS parses to a safe numeric value
- resolution is present

The right pattern is not a modal dialog. The right pattern is:
- a quiet readiness checklist
- inline hints under invalid fields
- CTA enabled only when the session is valid

### 21.2 During recording

The user needs three classes of information:

Critical:
- recording state
- elapsed duration
- dropped-frame warning

Operational:
- live preview
- current FPS
- throughput

Contextual:
- output destination
- selected camera
- format

Only the critical layer should visually dominate.

### 21.3 After recording

The current flow stops capture but does not clearly guide the user.

The redesign should show one of these outcomes:

If auto-decode is on:
- state chip changes to `Decoding`
- progress card appears with clear copy
- `Stop Recording` disappears
- destination summary changes from raw session target to export target

If auto-decode is off:
- completion card appears with `Raw session saved`
- action dock offers `Reveal Session Folder`
- setup reopens for next run

### 21.4 Error handling

Error presentation hierarchy:

1. Field hint
- for invalid user input

2. Inline banner
- for recoverable workflow issues such as missing path or unavailable camera

3. Blocking sheet/dialog
- only for failed recording startup or critical stop/decode failure that requires acknowledgment

Logs remain useful, but they should never be the first place a user learns that recording failed.

## 22. Copywriting Direction

The current copy is acceptable but generic. A premium minimal product needs tighter language.

Recommended copy style:
- short
- calm
- literal
- no exclamation marks
- no emoji

Examples:

Instead of:
- `Ready for a new session`

Prefer:
- `Ready to record`

Instead of:
- `Capture is live. Monitor preview, throughput, and dropped frames.`

Prefer:
- `Recording now. Watch preview and capture health.`

Instead of:
- `The recording is complete. Native decoding is now generating the export.`

Prefer:
- `Recording finished. Preparing export.`

Instead of:
- `Attention required`

Prefer:
- `Needs attention`

## 23. QML Component Blueprint

Recommended file structure:

```text
cmd/micecam_ui/qml/
  App.qml
  theme/
    Theme.qml
    Metrics.qml
  components/
    AppHeader.qml
    StateChip.qml
    PreviewStage.qml
    SessionCard.qml
    HealthCard.qml
    MetricsCard.qml
    ActivityCard.qml
    ActionDock.qml
    PrimaryButton.qml
    SecondaryButton.qml
    SettingField.qml
    InlineBanner.qml
```

Responsibilities:

`App.qml`
- top-level scene composition
- width breakpoint switching

`Theme.qml`
- colors
- typography sizes
- radii
- spacing tokens

`PreviewStage.qml`
- idle placeholder
- live image
- recording overlays
- decode/completion overlay

`SessionCard.qml`
- editable setup in idle and ready states
- compact read-only summary in recording and decoding states

`HealthCard.qml`
- readiness and live health
- interpreted health states instead of raw metrics only

`ActivityCard.qml`
- recent events list
- severity tint
- optional expand action for full diagnostics

`ActionDock.qml`
- one dominant action
- one or two supporting summaries
- adapts to session state

## 24. Backend Contract Recommendations

The backend should expose UX-oriented state, not only technical state.

Recommended new properties on `PipelineController`:

- `selectedCameraName`
- `hasAvailableCamera`
- `outputPathValid`
- `sessionNameValid`
- `fpsValid`
- `canStartRecording`
- `isDecoding`
- `hasWarning`
- `hasDroppedFramesWarning`
- `recentEventsModel`
- `resolvedSessionPath`
- `resolvedExportPath`
- `recordingDurationText`

Recommended event structure:
- `timestamp`
- `severity`
- `category`
- `message`

This removes fragile parsing logic from QML and makes the UI truly declarative.

## 25. Reference QML Scene Skeleton

The following is not final code. It is a composition guide for implementation.

```qml
ApplicationWindow {
    color: Theme.appBackground

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.space20

        AppHeader {
            sessionTitle: pipeline.sessionName
            state: pipeline.sessionState
            subtitle: pipeline.statusHeadline
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: width >= 1180 ? desktopWorkspace : stackedWorkspace
        }

        ActionDock {
            state: pipeline.sessionState
            canStart: pipeline.canStartRecording
            outputSummary: pipeline.outputDir
            autoDecode: pipeline.autoDecode
        }
    }

    Component {
        id: desktopWorkspace

        RowLayout {
            spacing: Theme.space20

            PreviewStage {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.preferredWidth: 360
                spacing: Theme.space16

                SessionCard {}
                HealthCard {}
                MetricsCard {}
                ActivityCard {}
            }
        }
    }
}
```

## 26. Implementation Priority for the Next Engineering Pass

Recommended order:

1. Split the existing `main.qml` into component files and introduce a theme singleton.
2. Replace the scroll-based stacked `GroupBox` page with preview stage plus side rail plus action dock.
3. Add readiness and state properties to `PipelineController` so QML can stop inferring behavior from raw fields.
4. Replace raw `logMessages` presentation with a recent-activity model and an optional detailed diagnostics drawer.
5. Tune typography, spacing, color, and motion only after the layout and state model are correct.

This order matters. If styling happens before state and composition are corrected, the result will still feel like a skinned utility app.
