#!/bin/bash
# Build the MSX2 game from your own Side Pocket (Mega Drive) ROM.
#   build_msx.sh <side-pocket.md> <genesis-plus-gx-checkout> <msxgl-checkout>
# Needs: gcc, python3 + Pillow, node, and a Genesis Plus GX checkout (github.com/ekeeke/Genesis-Plus-GX),
# plus an MSXGL checkout (github.com/aoineko-fr/MSXgl).  Nothing derived from the ROM is committed:
# msx/gen and msx/out are in .gitignore.
set -e
ROM=$(realpath "$1"); GPGX=$(realpath "$2"); MSXGL=$(realpath "$3")
HERE=$(cd "$(dirname "$0")" && pwd); WORK=$(mktemp -d)

echo "== 1/5 build the emulator core (exports vram/cram so the dump script can read them)"
( cd "$GPGX" && make -f Makefile.libretro -j"$(nproc)" >/dev/null && \
  cc -o "$WORK/gpgx.so" -shared $(find core libretro -name '*.o') -lm )
cc -O1 -I"$GPGX/libretro/libretro-common/include" -o "$WORK/mdrun" "$HERE/emu/mdrun.c" -ldl

echo "== 2/5 play to the table and dump VRAM/CRAM"
mkdir -p "$WORK/out"
printf 'dump 1250 play\nshot 1250 play\n' | cat "$HERE/emu/reach_gameplay.txt" - > "$WORK/script.txt"
"$WORK/mdrun" "$WORK/gpgx.so" "$ROM" "$WORK/script.txt" "$WORK/out"

echo "== 3/5 extract the table and ball graphics"
python3 "$HERE/extract_assets.py" "$WORK/out/play" "$HERE/../msx/gen"
echo "== 4/5 turn them into C arrays"
python3 "$HERE/gen_c_assets.py" "$HERE/../msx/gen"

echo "== 5/5 build the MSX2 ROM with MSXGL"
( cd "$HERE/../msx" && node "$MSXGL/engine/script/js/build.js" )
echo "done: $(realpath "$HERE/../msx/out/sidepocket.rom")"
