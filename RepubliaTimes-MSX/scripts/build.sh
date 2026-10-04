#!/bin/bash
# Build the ROM: convert assets -> compile -> link -> ROM, copy to dist/
set -e
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
python3 tools/gen_assets.py
python3 tools/gen_music.py
rm -rf out
node "$BUILD_JS" 2>&1 | tee build/build.log | grep -E "error|Error|warning|Warning|Size|Seg\[|Success$" | grep -v "^.*Success" || true
if [ ! -f out/republia.rom ]; then echo "BUILD FAILED"; tail -40 build/build.log; exit 1; fi
mkdir -p dist
OUT_ROM="dist/republia-msx-$VERSION${RT_TAG:+-$RT_TAG}.rom"
cp out/republia.rom "$OUT_ROM"
cp out/republia.map "${OUT_ROM%.rom}.map" 2>/dev/null || true
sha256sum "$OUT_ROM"
ls -l out/republia.rom
