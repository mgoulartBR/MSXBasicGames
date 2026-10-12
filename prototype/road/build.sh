#!/bin/sh
# uso: ./build.sh <vsync 0|1> <speed> <saida.rom> [dispoff 0|1]
set -e
cd "$(dirname "$0")"
printf 'VSYNC equ %s\nSPEED equ %s\nDISPOFF equ %s\n' "$1" "$2" "${4:-0}" > config.inc
python3 -I gen_tables.py >/dev/null
pasmo --bin road.asm "$3"
