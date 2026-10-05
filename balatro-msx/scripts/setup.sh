#!/bin/bash
# Prepare the development sandbox: MSXgl (pinned), SDCC (bundled in MSXgl), openMSX, Python deps.
set -e
source "$(dirname "$0")/env.sh"
if [ ! -d "$MSXGL_DIR/.git" ]; then
  mkdir -p "$(dirname "$MSXGL_DIR")"
  GIT_LFS_SKIP_SMUDGE=1 git clone --depth 1 --branch "$MSXGL_TAG" https://github.com/aoineko-fr/msxgl "$MSXGL_DIR"
fi
HEAD_NOW=$(git -C "$MSXGL_DIR" rev-parse HEAD)
[ "$HEAD_NOW" = "$MSXGL_COMMIT" ] || echo "WARNING: MSXgl is at $HEAD_NOW, expected $MSXGL_COMMIT ($MSXGL_TAG)"
chmod +x "$MSXGL_DIR"/tools/sdcc/bin/* "$MSXGL_DIR"/tools/MSXtk/bin/MSX* 2>/dev/null || true
if ! command -v openmsx >/dev/null; then
  (sudo -n true 2>/dev/null && SUDO=sudo || SUDO=; $SUDO apt-get update -qq && $SUDO apt-get install -y -qq openmsx xvfb python3-pil python3-numpy)
fi
command -v node >/dev/null || { echo "Node.js is required (MSXgl build tool)"; exit 1; }
[ -n "$PY" ] || { echo "Need Python with Pillow+NumPy (apt install python3-pil python3-numpy)"; exit 1; }
echo "SDCC:    $("$MSXGL_DIR"/tools/sdcc/bin/sdcc --version | head -1 | cut -c1-12,150-)"
echo "openMSX: $(openmsx -h 2>&1 | head -1)"
echo "MSXgl:   $MSXGL_TAG ($HEAD_NOW)"
