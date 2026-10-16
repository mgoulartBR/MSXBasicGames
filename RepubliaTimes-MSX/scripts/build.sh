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
# Guard: the fixed code must end below A000h (the bank window) or it disappears when a data segment is
# mapped (found the hard way with the diagnostic build). Warn when the margin is small.
python3 - <<'PY' || { echo "BUILD FAILED: fixed code overlaps the A000h bank window"; exit 1; }
import re,sys
m=re.search(r'Seg\[0000\]: Lower=([0-9A-F]+)h Higher=([0-9A-F]+)h',open('build/build.log').read())
if m:
    hi=int(m.group(2),16)
    print("fixed code ends at %04Xh (limit 9FFFh, margin %d bytes)"%(hi,0x9FFF-hi))
    if hi>0x9FFF: sys.exit(1)
    if 0x9FFF-hi<256: print("WARNING: less than 256 bytes of margin below the bank window")
PY
mkdir -p dist
OUT_ROM="dist/republia-msx-$VERSION${RT_TAG:+-$RT_TAG}.rom"
cp out/republia.rom "$OUT_ROM"
cp out/republia.map "${OUT_ROM%.rom}.map" 2>/dev/null || true
sha256sum "$OUT_ROM"
ls -l out/republia.rom
