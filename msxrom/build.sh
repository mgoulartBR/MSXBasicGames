#!/bin/sh
# BUILD: requer SDCC (>=4.2), Python 3 + numpy + pillow. Saida: out/msxdoom.rom (ASCII16, use -romtype ASCII16)
set -e
cd "$(dirname "$0")"; mkdir -p out
python3 tools/make_assets.py
sdasz80 -plosgff -o out/crt0.rel src/crt0.s
sdcc -mz80 --opt-code-size -c -o out/main.rel src/main.c
sdcc -mz80 --no-std-crt0 --code-loc 0x8000 --data-loc 0xC000 -o out/msxdoom.ihx out/crt0.rel out/main.rel
python3 tools/mkrom.py out/msxdoom.ihx out/data_banks.bin out/msxdoom.rom
