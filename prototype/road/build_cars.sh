#!/bin/sh
# Pista REAL (ROM do usuário) + sprites de carros adversários.
# uso: ./build_cars.sh <rom_smgp.md> <pista 0..15> <speedf 0..255> <paldir 0|1> <saida.rom> [posicao_inicial]
set -e
cd "$(dirname "$0")"
python3 -I ../../tools/smgp/export_track.py "$1" "$2" track.inc
printf 'SPEEDF equ %s\nPALDIR equ %s\nILV equ 1\nHSHIFT equ 2\nSTARTPOS equ %s\n' "$3" "$4" "${6:-0}" > tconfig.inc
python3 -I gen_cars.py -1 >/dev/null
pasmo --bin road_cars.asm "$5"
