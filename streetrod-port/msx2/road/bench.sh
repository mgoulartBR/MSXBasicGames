#!/bin/sh
# uso: ./bench.sh "<flags de compilacao>" [png]  -> imprime fps medido no openMSX
cd "$(dirname "$0")"
./build.sh $1 >/dev/null 2>&1 || { echo build falhou; exit 1; }
export OUT=/tmp/out.$$ VRAM=/tmp/vram.$$ SHOT=/tmp/x.png SDL_AUDIODRIVER=dummy
rm -f $OUT
timeout 120 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart road.rom -script measure.tcl >/dev/null 2>&1
echo "$1 => $(cat $OUT 2>/dev/null)"
[ -n "$2" ] && python3 vram2png.py $VRAM $(cat $OUT.r2) "$2"
rm -f $OUT $OUT.r2 $VRAM
