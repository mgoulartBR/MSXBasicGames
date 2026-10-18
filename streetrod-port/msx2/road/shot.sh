#!/bin/sh
# uso: ./shot.sh "<flags>" "t1 t2 ..." prefixo  -> prefixo_N.png (build com piloto automatico)
cd "$(dirname "$0")"
./build.sh $1 >/dev/null 2>&1 || { echo build falhou; exit 1; }
export SHOTS="$2" VRAM=/tmp/shot.$$ SDL_AUDIODRIVER=dummy
timeout 100 xvfb-run -a openmsx -machine ${MACHINE:-C-BIOS_MSX2} -cart road.rom -script shot.tcl >/dev/null 2>&1
i=0; for t in $2; do python3 vram2png.py $VRAM.$i $(cat $VRAM.$i.r2) $3_$i.png; i=$((i+1)); done
rm -f $VRAM.*
