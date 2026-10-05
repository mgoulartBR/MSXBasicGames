#!/bin/bash
# Convert assets, compile, link, package. Output: out/balatro.rom, copied to dist/.
# Usage: scripts/build.sh [clean|rebuild] [DEBUG]
source "$(dirname "$0")/env.sh"
cd "$PROJ_DIR" || exit 1
if [ "$1" = "clean" ] || [ "$1" = "rebuild" ]; then rm -rf out emul; [ "$1" = "clean" ] && exit 0; fi
$PY tools/gen_assets.py || exit 1
$PY tools/gen_data.py || exit 1
LOG=out/build.log; mkdir -p out
node "$MSXGL_DIR/engine/script/js/build.js" "${@:2}" > "$LOG" 2>&1; RC=$?
sed 's/\x1b\[[0-9;]*m//g' "$LOG" | grep -E "warning|error|Error|ERROR|Failed" | grep -v "^$" | head -40
if [ $RC -ne 0 ] || [ ! -f out/$ROM_NAME.rom ]; then sed 's/\x1b\[[0-9;]*m//g' "$LOG" | tail -30; echo "BUILD FAILED"; exit 1; fi
mkdir -p dist build
cp out/$ROM_NAME.rom dist/$ROM_NAME-msx-$VERSION.rom
cp out/$ROM_NAME.map build/$ROM_NAME-$VERSION.map
$PY tools/memreport.py out/$ROM_NAME.map out/$ROM_NAME.rom || true
sha256sum dist/$ROM_NAME-msx-$VERSION.rom
