#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${1:-${ROOT_DIR}/build}"
DIST_DIR="${2:-${ROOT_DIR}/dist}"
VERSION="${3:-beta}"

APP_NAME="MiceCam"
APP_DIR="${DIST_DIR}/${APP_NAME}.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"
PLUGIN_ROOT="${RESOURCES_DIR}/3rdParty/bundled_plugins"
DMG_PATH="${DIST_DIR}/${APP_NAME}-${VERSION}-macos-arm64.dmg"
ICON_SRC="${ROOT_DIR}/specs/001-micecam-v2-rewrite/UIDesign/Icon.png"

require_file() {
  if [[ ! -f "$1" ]]; then
    echo "Required file is missing: $1" >&2
    exit 1
  fi
}

require_file "${BUILD_DIR}/cmd/micecam_ui/micecam_ui"
require_file "${BUILD_DIR}/cmd/plugins/micecam_ffmpeg/micecam_ffmpeg_plugin"
require_file "${BUILD_DIR}/cmd/plugins/micecam_oak/micecam_oak_plugin"
require_file "${ROOT_DIR}/3rdParty/bundled_plugins/micecam.ffmpeg/plugin.json"
require_file "${ROOT_DIR}/3rdParty/bundled_plugins/micecam.oak/plugin.json"

rm -rf "${APP_DIR}" "${DMG_PATH}"
mkdir -p "${MACOS_DIR}" \
         "${PLUGIN_ROOT}/micecam.ffmpeg/bin" \
         "${PLUGIN_ROOT}/micecam.oak/bin"

cp "${BUILD_DIR}/cmd/micecam_ui/micecam_ui" "${MACOS_DIR}/${APP_NAME}"
cp "${ROOT_DIR}/3rdParty/bundled_plugins/micecam.ffmpeg/plugin.json" \
   "${PLUGIN_ROOT}/micecam.ffmpeg/plugin.json"
cp "${ROOT_DIR}/3rdParty/bundled_plugins/micecam.oak/plugin.json" \
   "${PLUGIN_ROOT}/micecam.oak/plugin.json"
cp "${BUILD_DIR}/cmd/plugins/micecam_ffmpeg/micecam_ffmpeg_plugin" \
   "${PLUGIN_ROOT}/micecam.ffmpeg/bin/micecam-ffmpeg"
cp "${BUILD_DIR}/cmd/plugins/micecam_oak/micecam_oak_plugin" \
   "${PLUGIN_ROOT}/micecam.oak/bin/micecam-oak"

chmod +x "${MACOS_DIR}/${APP_NAME}" \
         "${PLUGIN_ROOT}/micecam.ffmpeg/bin/micecam-ffmpeg" \
         "${PLUGIN_ROOT}/micecam.oak/bin/micecam-oak"

# Generate app icon from source PNG
ICONSET_DIR="${RESOURCES_DIR}/${APP_NAME}.iconset"
rm -rf "${ICONSET_DIR}" "${RESOURCES_DIR}/${APP_NAME}.icns"
mkdir -p "${ICONSET_DIR}"

if [[ -f "${ICON_SRC}" ]] && command -v sips >/dev/null 2>&1; then
  # Generate all required icon sizes from source PNG (assumed 1024x1024)
  for size in 16 32 64 128 256 512; do
    sips -z ${size} ${size} "${ICON_SRC}" --out "${ICONSET_DIR}/icon_${size}x${size}.png" >/dev/null 2>&1
    sips -z $((size * 2)) $((size * 2)) "${ICON_SRC}" --out "${ICONSET_DIR}/icon_${size}x${size}@2x.png" >/dev/null 2>&1
  done
  sips -z 1024 1024 "${ICON_SRC}" --out "${ICONSET_DIR}/icon_512x512@2x.png" >/dev/null 2>&1

  iconutil -c icns "${ICONSET_DIR}" -o "${RESOURCES_DIR}/${APP_NAME}.icns"
  rm -rf "${ICONSET_DIR}"
  echo "App icon generated: ${RESOURCES_DIR}/${APP_NAME}.icns"
else
  echo "Warning: sips not available or icon source missing, skipping icon generation"
fi

cat > "${CONTENTS_DIR}/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key>
  <string>${APP_NAME}</string>
  <key>CFBundleIdentifier</key>
  <string>com.neuralgrid.micecam</string>
  <key>CFBundleName</key>
  <string>${APP_NAME}</string>
  <key>CFBundleIconFile</key>
  <string>${APP_NAME}.icns</string>
  <key>CFBundleDisplayName</key>
  <string>${APP_NAME}</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleShortVersionString</key>
  <string>${VERSION}</string>
  <key>CFBundleVersion</key>
  <string>${VERSION}</string>
  <key>LSMinimumSystemVersion</key>
  <string>13.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
  <key>NSSupportsAutomaticGraphicsSwitching</key>
  <true/>
</dict>
</plist>
EOF

if command -v macdeployqt >/dev/null 2>&1; then
  macdeployqt "${APP_DIR}" -always-overwrite
elif [[ -x "$(brew --prefix qt@6 2>/dev/null)/bin/macdeployqt" ]]; then
  "$(brew --prefix qt@6)/bin/macdeployqt" "${APP_DIR}" -always-overwrite
else
  echo "macdeployqt is missing. Install Qt 6 or add macdeployqt to PATH." >&2
  exit 1
fi

hdiutil create \
  -volname "${APP_NAME} ${VERSION}" \
  -srcfolder "${APP_DIR}" \
  -ov \
  -format UDZO \
  "${DMG_PATH}"

echo "${DMG_PATH}"
