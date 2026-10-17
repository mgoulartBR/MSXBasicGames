#!/bin/sh
# uso: ./build.sh [-DSTEP=2 -DVSYNC=1 ...]  -> road.rom (16 KB)
set -e
cd "$(dirname "$0")"
python3 gen_tables.py >/dev/null
# constantes compartilhadas com o assembly (derivadas dos -D recebidos)
SH=$(echo "$@" | sed -n 's/.*-DSTEP_SH=\([0-9]*\).*/\1/p'); SH=${SH:-1}
NR=$(echo "$@" | sed -n 's/.*-DN_ROWS=\([0-9]*\).*/\1/p'); NR=${NR:-15}
VB=$(echo "$@" | sed -n 's/.*-DVIEW_BOT=\([0-9]*\).*/\1/p'); VB=${VB:-112}
DM=$(echo "$@" | sed -n 's/.*-DDRAW_MODE=\([0-9]*\).*/\1/p'); DM=${DM:-0}
printf '\t.equ STEP, %s\n\t.equ STEP_SH, %s\n\t.equ NROWS, %s\n\t.equ VIEW_BOT, %s\n\t.equ DRAW_MODE, %s\n' $((1<<SH)) $SH $NR $VB $DM > config.inc
sdasz80 -plosgff crt0.rel crt0.s
sdasz80 -plosgff vdpasm.rel vdpasm.s
sdasz80 -plosgff proj.rel proj.s
sdcc -mz80 --std-sdcc11 --opt-code-speed -c "$@" ${SRC:-main.c} -o $(basename ${SRC:-main.c} .c).rel
sdcc -mz80 --no-std-crt0 --code-loc 0x4020 --data-loc 0xC000 crt0.rel $(basename ${SRC:-main.c} .c).rel vdpasm.rel proj.rel -o road.ihx
python3 ihx2rom.py road.ihx road.rom
