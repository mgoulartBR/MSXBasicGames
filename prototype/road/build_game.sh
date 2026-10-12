#!/bin/sh
# Protótipo "game" v6: pista real (ROM do usuário) + carros + layout de 3 regiões.
# uso: ./build_game.sh <rom_smgp.md> <pista 0..15> <speedf 0..255> <paldir 0|1> <saida.rom> [posicao_inicial]
set -e
cd "$(dirname "$0")"
python3 -I ../../tools/smgp/export_track.py "$1" "$2" track.inc
printf 'SPEEDF equ %s\nPALDIR equ %s\nILV equ 1\nHSHIFT equ 2\nSTARTPOS equ %s\n' "$3" "$4" "${6:-0}" > tconfig.inc
python3 -I gen_game.py -1 >/dev/null
pasmo --bin game.asm "$5"
# completa a ROM até 32 KB (cartucho "Normal": páginas 1 e 2); falha se passar do limite
python3 -I - "$5" <<'PY'
import sys
p = sys.argv[1]
d = open(p, "rb").read()
assert len(d) <= 32768, "ROM maior que 32 KB: %d bytes" % len(d)
open(p, "wb").write(d + bytes(32768 - len(d)))
print("ROM: %d bytes de código/dados + preenchimento até 32768" % len(d))
PY
