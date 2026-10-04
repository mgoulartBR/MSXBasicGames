#!/bin/bash
# Shared environment for all scripts. Pinned toolchain versions live here.
export RT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export MSXGL_TAG="v1.5.0"
export MSXGL_COMMIT="be3278424ec6a1dcd23563564c857e93aaec090d"
export MSXGL_DIR="${MSXGL_DIR:-$RT_ROOT/MSXgl}"
export BUILD_JS="$MSXGL_DIR/engine/script/js/build.js"
export ROM_NAME="republia"
export VERSION="0.1"
