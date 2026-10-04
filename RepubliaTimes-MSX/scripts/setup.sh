#!/bin/bash
# Reproducible environment: MSXgl (pinned tag, bundles SDCC) + openMSX + python deps.
set -e
source "$(dirname "$0")/env.sh"
if [ ! -d "$MSXGL_DIR/.git" ]; then
  git clone --depth 1 --branch "$MSXGL_TAG" https://github.com/aoineko-fr/MSXgl.git "$MSXGL_DIR"
fi
ACTUAL=$(git -C "$MSXGL_DIR" rev-parse HEAD)
[ "$ACTUAL" = "$MSXGL_COMMIT" ] || { echo "MSXgl commit mismatch: $ACTUAL != $MSXGL_COMMIT"; exit 1; }
command -v node >/dev/null || { echo "node is required (MSXgl build scripts)"; exit 1; }
if ! command -v openmsx >/dev/null; then
  echo "Installing openMSX (apt)..."; (apt-get update -qq && apt-get install -y -qq openmsx openmsx-data) || echo "WARN: install openMSX manually"
fi
python3 -c "import PIL, numpy" 2>/dev/null || pip install -q pillow numpy
echo "SDCC: $("$MSXGL_DIR/tools/sdcc/bin/sdcc" --version | head -1 | sed 's/.*TD- //')"
echo "openMSX: $(openmsx -v 2>&1 | head -1)"
echo "MSXgl: $MSXGL_TAG ($ACTUAL)"
