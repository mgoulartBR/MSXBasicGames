#!/bin/sh
# uso: ./build.sh [-DSTEP=2 -DVSYNC=1 ...]  -> road.rom (32 KB: codigo + assets)
set -e
cd "$(dirname "$0")"
python3 gen_tables.py >/dev/null
python3 gen_assets.py
# constantes compartilhadas com o assembly (derivadas dos -D recebidos)
SH=$(echo "$@" | sed -n 's/.*-DSTEP_SH=\([0-9]*\).*/\1/p'); SH=${SH:-2}
NR=$(echo "$@" | sed -n 's/.*-DN_ROWS=\([0-9]*\).*/\1/p'); NR=${NR:-15}
VB=$(echo "$@" | sed -n 's/.*-DVIEW_BOT=\([0-9]*\).*/\1/p'); VB=${VB:-95}
DM=$(echo "$@" | sed -n 's/.*-DDRAW_MODE=\([0-9]*\).*/\1/p'); DM=${DM:-2}
printf '\t.equ STEP, %s\n\t.equ STEP_SH, %s\n\t.equ NROWS, %s\n\t.equ VIEW_BOT, %s\n\t.equ DRAW_MODE, %s\n\t.equ CX, %s\n' $((1<<SH)) $SH $NR $VB $DM ${CX:-160} > config.inc
sdasz80 -plosgff crt0.rel crt0.s
sdasz80 -plosgff vdpasm.rel vdpasm.s
sdasz80 -plosgff proj.rel proj.s
B=$(basename ${SRC:-main.c} .c)
sdcc -mz80 --std-sdcc11 ${OPT:---opt-code-speed} -S "$@" ${SRC:-main.c} -o $B.asm 2>&1 | grep -v "instructionSize\|^'b_\|^$" || true
python3 bankify.py $B.asm ${SRC:-main.c} ${B}_b.asm
sdasz80 -plosgff $B.rel ${B}_b.asm
BKFLAGS=$(grep -o 'area BANK[0-9]*' ${B}_b.asm | sort -u | sed 's/area BANK//' | while read n; do printf -- '-Wl-bBANK%s=0x%X8000 ' $n $n; done)
# nucleo em 0x4020 (banco 0, pagina 1); bancos de codigo 1..N em 0x8000 (endereco virtual = banco<<16 | 0x8000; ihx2rom reposiciona)
sdcc -mz80 --no-std-crt0 --code-loc 0x4020 --data-loc 0xC000 $BKFLAGS crt0.rel vdpasm.rel proj.rel $B.rel -o road.ihx
python3 ihx2rom.py road.ihx road.rom assets.bin
