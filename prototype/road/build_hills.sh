#!/bin/sh
# uso: ./build_hills.sh <speed> <paldir> <forcecurve> <forcehill> <ilv 0|1> <iladj> <saida.rom>
set -e
cd "$(dirname "$0")"
printf 'SPEED equ %s\nPALDIR equ %s\nFORCECURVE equ %s\nFORCEHILL equ %s\nILV equ %s\nHILLPH equ 20\n' "$1" "$2" "$3" "$4" "$5" > hconfig.inc
python3 -I gen_hills.py "$6" >/dev/null
pasmo --bin road_hills.asm "$7"
