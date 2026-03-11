#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
APP_BIN="${BUILD_DIR}/micecam_ui"

if [[ ! -x "${APP_BIN}" ]]; then
  echo "micecam_ui is missing. Build it first with:"
  echo "  cmake --build build --target micecam_ui -j4"
  exit 1
fi

WORKER_OUTPUT="$(printf '{"type":"shutdown"}\n' | "${APP_BIN}" --worker)"

if [[ "${WORKER_OUTPUT}" != *'"type":"hello"'* ]]; then
  echo "worker handshake failed"
  exit 1
fi

echo "worker handshake ok"
echo "${WORKER_OUTPUT}"
