# UI Design Spec: MiceCam v2

## Metadata

| Field      | Value                          |
|------------|--------------------------------|
| Feature ID | `001-micecam-v2-rewrite-ui`    |
| Branch     | `feat/v2-rewrite`              |
| Status     | Draft                          |
| Owner      | `jingyi`                       |
| Date       | `2026-05-13`                   |

## Summary

Native Qt/QML desktop UI following Apple Human Interface Guidelines. Clean, system-level aesthetic with navy blue accent. Camera preview grid with adaptive layout, global recording controls, real-time status monitoring, and comprehensive session statistics display. Designed for laboratory researchers monitoring multiple animal cameras simultaneously.

## Design Principles

1. **Content First** — camera previews are the hero; UI chrome is minimal and recedes
2. **Direct Manipulation** — drag to rearrange, click to zoom, single global record button
3. **Clarity at a Glance** — status colors (green/amber/red) communicate system health instantly
4. **No Surprises** — every destructive action confirms; every failure explains itself
5. **Apple HIG** — SF fonts, SF Symbols, rounded corners, light shadows, navigation patterns

## Visual Language

### Colors

| Token | Value | Usage |
|-------|-------|-------|
| `navy-primary` | `#1B2A4A` | Main accent: buttons, selected states, highlights |
| `navy-light` | `#2D4373` | Hover states |
| `navy-dark` | `#0F1A2E` | Pressed states |
| `record-red` | `#FF3B30` | Recording button, recording indicator dot |
| `status-green` | `#34C759` | Connection healthy, metrics normal |
| `status-amber` | `#FF9500` | Warning thresholds exceeded |
| `status-red` | `#FF3B30` | Error: disconnect, failure, stall |
| `bg-primary` | `#FFFFFF` | Main background (light mode) |
| `bg-secondary` | `#F2F2F7` | Sidebar, panels, card backgrounds |
| `bg-tertiary` | `#E5E5EA` | Dividers, subtle separators |
| `text-primary` | `#1C1C1E` | Headlines, body text |
| `text-secondary` | `#6E6E73` | Captions, labels, secondary metadata |
| `text-tertiary` | `#AEAEB2` | Disabled, placeholder text |
| `overlay-bg` | `rgba(0,0,0,0.40)` | Semi-transparent overlay on camera grids |

### Typography

All text uses San Francisco system fonts via `font.family: "SF Pro Display"` / `font.family: "SF Pro Text"`:

| Token | Size | Weight | Usage |
|-------|------|--------|-------|
| `ui-large-title` | 28pt | Bold | Window title, major headings |
| `ui-title` | 20pt | Semibold | Sidebar headers, panel titles |
| `ui-headline` | 16pt | Semibold | Camera labels, section headers |
| `ui-body` | 14pt | Regular | Body text, descriptions, settings |
| `ui-caption` | 12pt | Regular | Status labels, secondary info |
| `ui-caption-bold` | 12pt | Semibold | Data values, emphasis in stats |
| `ui-mono` | 13pt | Regular (SF Mono) | Frame counts, timestamps, data tables |

### Icons

SF Symbols v5, weight: Regular (default) / Semibold (toolbar). Key symbols:
- `video.fill` — camera
- `record.circle` — recording control
- `gearshape.fill` — settings
- `rectangle.grid.2x2` — grid view
- `rectangle.fill` — fullscreen camera
- `exclamationmark.triangle.fill` — alert
- `antenna.radiowaves.left.and.right` — connection status
- `folder.fill` — output directory
- `clock.fill` — session duration
- `gauge.with.dots.needle.33percent` — disk usage

### Layout Metrics

| Token | Value | Usage |
|-------|-------|-------|
| `spacing-xs` | 4pt | Tight icon+label groups |
| `spacing-sm` | 8pt | Card internal padding |
| `spacing-md` | 12pt | Standard element spacing |
| `spacing-lg` | 16pt | Section spacing |
| `spacing-xl` | 24pt | Major layout divisions |
| `radius-sm` | 6pt | Small UI elements, buttons |
| `radius-md` | 10pt | Cards, panels |
| `radius-lg` | 14pt | Large containers |
| `shadow-light` | 0 1px 3px rgba(0,0,0,0.08) | Card elevation |
| `shadow-medium` | 0 2px 8px rgba(0,0,0,0.12) | Dropdown, popover |

## Layout Structure

```
┌─────────────────────────────────────────────────────────┐
│ Toolbar                                                  │
│ [Record] [Stop] [Settings ⚙] [Fullscreen ⛶]             │
├──────────┬──────────────────────────────────────────────┤
│ Sidebar  │  Camera Preview Grid                          │
│          │                                               │
│ 📷 CAM_A │  ┌──────────────┐ ┌──────────────┐          │
│ 📷 CAM_B │  │              │ │              │          │
│ 📷 CAM_C │  │   CAM_A      │ │   CAM_B      │          │
│ 📷 CAM_D │  │   ● 29.97fps │ │   ● 29.97fps │          │
│ 📷 USB-1 │  │   0 drops    │ │   0 drops    │          │
│          │  └──────────────┘ └──────────────┘          │
│ Settings │  ┌──────────────┐ ┌──────────────┐          │
│ ▸ Encoder│  │              │ │              │          │
│ ▸ Alert  │  │   CAM_C      │ │   CAM_D      │          │
│          │  │   ● 29.97fps │ │   ● 29.97fps │          │
│          │  │   0 drops    │ │   0 drops    │          │
│          │  └──────────────┘ └──────────────┘          │
│          │  ┌──────────────┐                            │
│          │  │              │                            │
│          │  │   USB Cam    │                            │
│          │  │   ● 29.97fps │                            │
│          │  │   0 drops    │                            │
│          │  └──────────────┘                            │
├──────────┴──────────────────────────────────────────────┤
│ Status Bar                                               │
│ ⏱ 00:42:17  │  📊 76,230 frames  │  29.97 fps avg      │
│ 💾 3.2 GB  │  ⚠ 45% disk remaining                     │
└─────────────────────────────────────────────────────────┘
```

### Adaptive Grid Behavior

Grid columns adapt to stream count:

| Stream count | Layout | Card size |
|---|---|---|
| 1 | 1×1 full width | Fills main area |
| 2 | 1×2 (side by side) | Equal halves |
| 3 | 1+2 stacked | Top: full width; Bottom: 2 half-width |
| 4 | 2×2 | Equal quarters |
| 5 | 2+3 stacked | Top: 2 half-width; Bottom: 3 thirds |
| 6 | 2×3 | Sixths |
| 7+ | 3×3 with overflow | Responsive |

Each card maintains 16:9 aspect ratio. Gaps between cards: `spacing-sm` (8pt).

### Card Anatomy

```
┌─────────────────────────────┐
│ CAM_A                  ● REC│  ← Camera label + recording indicator
│                             │
│                             │
│       [Camera Preview]      │  ← 16:9 video feed, fills card
│                             │
│                             │
│                              │
│ 29.97 fps    0 drops  ⚠     │  ← Semi-transparent overlay bar
└─────────────────────────────┘     (appears on hover or always on during recording)
```

Overlay bar: `overlay-bg` backdrop blur, white text. Always visible during recording, shows on hover during idle.

## User Scenarios

### US-UI-001: Application Launch and Camera Discovery

**Priority**: P1

**Independent Test**: Launch application with no cameras. Verify empty state. Connect OAK-D. Verify 4 cameras appear in grid within 3 seconds. Verify labels match camera IDs.

**Acceptance Scenarios**:
- Given no cameras connected, When application launches, Then main area shows centered empty state: SF Symbol `video.slash.fill` (64pt, `text-tertiary`), text "No Cameras Detected" (`ui-title`, `text-secondary`), subtext "Connect a camera or check Settings" (`ui-body`, `text-tertiary`)
- Given camera backend scan completes, When cameras are found, Then grid updates with card per stream; spinner or brief transition (fade in, 300ms)
- Given a camera backend fails silently, When application starts, Then sidebar shows backend name grayed out with "Unavailable" badge; other backends function normally
- Given user connects new camera while app is running, When backend detects new device, Then new card appears with slide-in animation (300ms, from bottom)

### US-UI-002: Global Recording Controls

**Priority**: P1

**Independent Test**: Click Record button. Verify all camera cards show red recording dot. Verify toolbar button changes to Stop. Click Stop. Verify output files written.

**Acceptance Scenarios**:
- Given cameras are connected and preflight passes, When user clicks Record button (red, `record-red`, SF Symbol `record.circle`, label "Record"), Then all camera cards show pulsing red dot in top-right corner, Record button transforms to Stop (dark fill, SF Symbol `stop.fill`), session timer starts in status bar
- Given recording is active, When user clicks Stop button, Then confirmation dialog appears: "Stop Recording? Current session will be saved." with "Continue Recording" / "Stop & Save" buttons; on confirm, all MP4 files finalize, stats write, cards return to idle state
- Given recording is active, When user clicks Record button (now Stop), Then recording stops immediately (no confirmation if already recording)
- Given preflight fails (disk space), When user clicks Record, Then modal appears: SF Symbol `exclamationmark.triangle.fill` (amber), preflight failure details, "Adjust Settings" button; Record does not start
- Given app is recording, When user attempts to close application, Then system dialog: "Recording in progress. Stop recording and exit?" with "Continue Recording" / "Stop & Exit"

### US-UI-003: Camera Configuration

**Priority**: P1

**Independent Test**: Click on CAM_A card settings. Change resolution from 1080p to 720p. Verify change is saved. Start recording. Verify CAM_A records at 720p.

**Acceptance Scenarios**:
- Given camera cards are displayed, When user right-clicks or long-presses a card, Then context menu appears: "Configure," "Fullscreen," "Remove" (greyed if minimum 1 stream)
- Given "Configure" is selected, When configuration panel opens (replace sidebar or popover), Then panel shows: camera name (readonly), resolution picker (dropdown with supported resolutions), framerate picker (dropdown with supported rates), format hint (readonly, e.g., "MJPEG / UYVY422")
- Given user changes resolution or framerate, When selection is different from active recording setting, Then "Apply" button enables; on click, camera reinitializes at new settings
- Given settings panel is open, When user clicks Record, Then panel auto-closes (or settings are locked during recording)

### US-UI-004: Fullscreen Single Camera View

**Priority**: P2

**Independent Test**: Double-click CAM_A card. Verify CAM_A fills entire main area. Verify other cameras appear as thumbnails at bottom. Double-click again. Verify returns to grid.

**Acceptance Scenarios**:
- Given grid view, When user double-clicks a camera card, Then card expands to fill main area with zoom transition (300ms, ease-in-out); other cards minimize to a horizontal thumbnail strip at bottom (80px height, horizontal scroll); toolbar still visible
- Given fullscreen view, When a camera disconnects, Then fullscreen exits with error transition; grid view restores
- Given fullscreen view, When user double-clicks or presses Escape, Then camera returns to grid position with reverse zoom transition (300ms)
- Given fullscreen view is active, When user clicks Record, Then recording indicator appears on fullscreen card; thumbnail strip shows recording dots on active streams

### US-UI-005: Status Bar

**Priority**: P1

**Independent Test**: Start recording. Verify status bar shows elapsed time, frame count, fps, file size, disk remaining. Inject high drop rate. Verify drop rate turns amber, then red as threshold crosses.

**Acceptance Scenarios**:
- Given recording is not active, When application is idle, Then status bar shows: camera count (e.g., "5 cameras"), disk free space, no time/fps/recording-specific metrics
- Given recording is active, When session is running, Then status bar shows (left to right): [elapsed time `ui-mono`] | [total frames `ui-mono`] | [average fps `ui-mono`] | [total file size `ui-mono`] | [disk remaining % with gauge icon]; updates at 1Hz
- Given drop rate exceeds 0.1%, When status bar updates, Then frame count text turns amber; drop rate displayed as amber badge; when >1%, turns red
- Given disk remaining < 10%, When status bar updates, Then disk gauge turns amber with warning icon; when < 5%, turns red with "Low Disk" label

### US-UI-006: Alert Notifications

**Priority**: P2

**Independent Test**: Trigger a camera disconnect during recording. Verify red alert banner appears at top of grid. Verify alert auto-dismisses after 5 seconds or on click.

**Acceptance Scenarios**:
- Given any alert condition fires (camera disconnect, high drop rate, encoder fallback, pipeline stall), When alert is generated, Then banner appears at top of main area: SF Symbol icon (color matches severity), alert message (`ui-body`), timestamp (`ui-caption`); slides in from top (200ms)
- Given alert is displayed, When user clicks alert or 5 seconds elapse, Then alert slides out (200ms); all alerts remain visible in alert history (accessible from toolbar bell icon)
- Given multiple alerts fire, When alerts stack, Then most recent appears first; max 3 visible simultaneously; older alerts collapse to "N more alerts" indicator
- Given alert history is opened (toolbar bell icon), When user views history, Then scrollable list of all alerts with timestamps, severity colors, and source stream; "Clear All" button at bottom
- Given no Feishu webhook configured, When alert fires, Then a small "Configure alerts" link appears in alert banner leading to Settings > Alerts

### US-UI-007: Sidebar Camera List

**Priority**: P2

**Independent Test**: Launch app with 5 cameras. Verify sidebar lists all 5 with status indicators. Click a camera name — verify it scrolls into view in grid.

**Acceptance Scenarios**:
- Given cameras are connected, When application is running, Then sidebar (240pt width, resizable) shows: section header "Cameras" (`ui-headline`), clickable camera list items with status dot (green=connected, red=disconnected, grey=idle-not-recording)
- Given user clicks camera name in sidebar, When camera card is not visible in current grid scroll position, Then grid scrolls smoothly to bring card into view; card briefly highlights (navy-light border, 300ms)
- Given 0 cameras, When sidebar is empty, Then "No cameras" placeholder text appears with link to open camera settings
- Given sidebar width is resized by dragging right edge, When width < 180pt, Then camera names truncate with ellipsis; when > 400pt, then extra detail appears (resolution, format)

### US-UI-008: Settings Panel

**Priority**: P2

**Independent Test**: Click gear icon. Verify settings panel slides in from right or replaces main area. Change watchdog timeout. Return to main view. Verify setting persisted.

**Acceptance Scenarios**:
- Given user clicks Settings button (toolbar gear icon), When settings panel opens, Then sidebar or main area transitions to settings view (push transition, 300ms); back button ("< Cameras") returns to main view
- Given settings view, When user navigates sections, Then settings organized in list with disclosure indicators: Encoding, Alerts, Logging, About; tap to expand section inline
- Given Encoding section, When user taps, Then expand to show: default bitrate slider (3-10 Mbps, `navy-primary` track), keyframe interval (stepper, 30-300 frames)
- Given Alerts section, When user taps, Then expand to show: Feishu webhook URL (text field, masked), watchdog timeout (stepper, 1-10 seconds), alert severity thresholds (Yellow/Red sliders for drop rate %)
- Given Logging section, When user taps, Then expand to show: log level picker (Trace/Debug/Info/Warn/Error), "Open Log Folder" button
- Given About section, When user taps, Then expand to show: MiceCam version, build info, FFmpeg version, DepthAI version, license link
- Given any setting is changed, When user returns to main view, Then setting is auto-saved (no explicit Save button)

### US-UI-009: Session History (Post-Recording)

**Priority**: P3

**Independent Test**: Complete a recording session. Verify session appears in history list. Click session. Verify metadata and stats are displayed read-only.

**Acceptance Scenarios**:
- Given recording session completes (user stops or session ends), When session is finalized, Then session entry appears in sidebar "History" section (or separate History tab): session date/time, duration, stream count, total size, alert count badge
- Given user clicks a history entry, When session detail opens, Then read-only view shows: mini grid of camera thumbnails, per-stream stats table (frames, drops, encoder, bitrate), timeline of alerts during session, "Open Folder" button to reveal output directory in system file manager
- Given history list grows, When list exceeds sidebar height, Then shows "Recent" (last 5) with "Show All..." link at bottom; full history in separate view

### US-UI-010: Empty States and Error States

**Priority**: P2

**Independent Test**: Verify each empty/error state renders correctly with appropriate icon, message, and action.

**Acceptance Scenarios**:
- Given no cameras detected, When launching app, Then empty state: `video.slash.fill`, "No Cameras Detected", "Connect a camera or check Settings", [Open Settings] button
- Given encoder fails during recording, When error occurs, Then modal: `exclamationmark.triangle.fill` (red), "Encoding Error", detailed error message, encoder fallback notification if applicable, [Continue] button (resumes with fallback) or [Stop Recording] button
- Given output directory is not writable, When user clicks Record, Then modal: `folder.fill.badge.minus`, "Cannot Write to Output Directory", path display, [Choose Different Folder] button
- Given OAK device is locked by another instance, When app tries to use it, Then card shows lock badge, tooltip: "Device in use by another application"; camera is not selectable for recording

## Component Hierarchy

```
ApplicationWindow
├── Toolbar
│   ├── RecordButton (primary action, context-sensitive)
│   ├── StopButton (visible during recording)
│   ├── Spacer
│   ├── AlertBell (badge with unread count)
│   └── SettingsButton (gear)
├── SplitView
│   ├── Sidebar (240pt default, resizable)
│   │   ├── CameraListSection
│   │   │   ├── SectionHeader "Cameras"
│   │   │   └── CameraListItem[] (icon, name, status dot, clickable)
│   │   ├── HistorySection (P3)
│   │   │   ├── SectionHeader "History"
│   │   │   └── HistoryListItem[] (date, duration, alert badge)
│   │   └── SettingsNavigation (when settings open)
│   │       ├── EncodingSection
│   │       ├── AlertsSection
│   │       ├── LoggingSection
│   │       └── AboutSection
│   └── MainContentArea (StackView)
│       ├── EmptyStateView
│       ├── CameraGridView
│       │   └── CameraCard[]
│       │       ├── CameraPreview (16:9 video)
│       │       ├── CameraLabel (top-left)
│       │       ├── RecordingIndicator (top-right, pulsing red dot)
│       │       └── StatusOverlay (bottom bar, semi-transparent)
│       │           ├── FpsLabel
│       │           ├── DropCountLabel
│       │           └── AlertIcon (conditional)
│       ├── FullscreenCameraView
│       │   ├── FullscreenPreview
│       │   └── ThumbnailStrip (bottom)
│       ├── SessionHistoryDetailView (P3)
│       └── AlertBanner (overlay, top of content area)
├── StatusBar
│   ├── DurationLabel (only during recording)
│   ├── FrameCountLabel
│   ├── FpsAvgLabel
│   ├── FileSizeLabel
│   └── DiskGauge
└── Modals (conditional)
    ├── PreflightFailureModal
    ├── StopRecordingConfirmModal
    ├── EncoderErrorModal
    └── CloseWhileRecordingModal
```

## Interaction Patterns

| Pattern | Implementation |
|---------|---------------|
| Button press | Scale 0.97 on press, spring back 1.0, 150ms |
| View transitions | Cross-dissolve 200ms for settings, push 300ms for navigation |
| Card hover | Shadow increase + scale 1.02, 150ms |
| Alert appearance | Slide from top, 200ms, opacity fade-in |
| Card add/remove | Scale + fade, 300ms, staggered for batch operations |
| Recording indicator | Pulse animation: opacity 1.0 ↔ 0.4, 1s cycle |
| Scroll | Native ScrollView with momentum |
| Resize | SplitView handle drag, live preview resize |
| Tooltips | 500ms hover delay, 12pt SF Pro, light shadow |

## Animation Tokens

| Token | Value |
|-------|-------|
| `duration-instant` | 100ms |
| `duration-fast` | 200ms |
| `duration-normal` | 300ms |
| `duration-slow` | 500ms |
| `easing-default` | `ease-in-out` |
| `easing-spring` | `spring(0.4, 0.8, 1.0)` |

## Accessibility

- All buttons have accessible labels and tooltips
- Status colors are always paired with text or icon (not color-only)
- Keyboard navigation: Tab between controls, Space/Enter to activate, Escape to dismiss modals / exit fullscreen
- Minimum touch target: 44pt × 44pt (Apple HIG)
- Dynamic Type support via Qt system font scaling
- High contrast mode: increased border weight and icon contrast

## Out of Scope

- Dark mode (light mode only for v2)
- Custom window chrome (use platform default title bar)
- Drag-and-drop camera rearrangement (fixed grid based on connection order)
- Multi-window support
- Touch Bar support
- VoiceOver / full screen reader (basic accessibility only)
- Localization (English only)
- Video playback within the app (use system player for `.mp4` files)

## Risk Notes

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Qt/QML performance with 5 simultaneous video previews | Medium | High | Use hardware-accelerated QML rendering; limit preview to decoded keyframes at lower resolution for preview if needed |
| SF Symbols rendering in Qt | High | Medium | Bundle SVG icons as fallback; test SF font rendering on all platforms |
| Cross-platform font consistency (SF Pro not available on Windows/Linux) | High | Medium | Fall back to Segoe UI (Windows) / Noto Sans (Linux); detect platform at runtime |
| Live preview decode latency causing UI jank | Medium | Medium | Decode preview frames on background thread; update QML via image provider with double-buffering |
