#!/bin/sh
# uso: ./build_zones.sh <speed> <paldir 0|1> <forcecurve> <iladj> <saida.rom>
set -e
cd "$(dirname "$0")"
printf 'SPEED equ %s\nPALDIR equ %s\nFORCECURVE equ %s\n' "$1" "$2" "$3" > zconfig.inc
python3 -I gen_zones.py "$4" >/dev/null
pasmo --bin road_zones.asm "$5"
