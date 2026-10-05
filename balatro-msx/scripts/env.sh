#!/bin/bash
# Shared environment for the Balatro-MSX scripts.
PROJ_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MSXGL_TAG="v1.5.0"
MSXGL_COMMIT="be3278424ec6a1dcd23563564c857e93aaec090d"
# MSXgl checkout: $MSXGL_DIR, else ./toolchain/msxgl, else the sandbox default location
if [ -z "$MSXGL_DIR" ]; then
  if [ -d "$PROJ_DIR/toolchain/msxgl" ]; then MSXGL_DIR="$PROJ_DIR/toolchain/msxgl"
  elif [ -d /home/user/aoineko-fr/msxgl ]; then MSXGL_DIR=/home/user/aoineko-fr/msxgl
  else MSXGL_DIR="$PROJ_DIR/toolchain/msxgl"; fi
fi
export PROJ_DIR MSXGL_DIR MSXGL_TAG MSXGL_COMMIT
ROM_NAME="balatro"
VERSION="$(cat "$PROJ_DIR/VERSION" 2>/dev/null || echo 0.0)"
export ROM_NAME VERSION
# Python interpreter that has Pillow + NumPy (the sandbox default python3 is 3.11 but apt's Pillow targets 3.12)
if [ -z "$PY" ]; then
  for p in python3 python3.12 python3.13 python3.11; do
    if command -v $p >/dev/null && $p -c "import PIL, numpy" 2>/dev/null; then PY=$p; break; fi
  done
fi
export PY
