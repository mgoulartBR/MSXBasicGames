#!/bin/sh
# BUILD: requer SDCC (>=4.2), makebin e Python 3. Saida: out/msxdoom.rom (16 KiB, 0x4000-0x7FFF)
set -e
cd "$(dirname "$0")"; mkdir -p out
python3 tools/gen_level.py
sdasz80 -plosgff -o out/crt0.rel src/crt0.s
sdcc -mz80 --opt-code-size -c -o out/main.rel src/main.c
sdcc -mz80 --no-std-crt0 --code-loc 0x4010 --data-loc 0xC000 -o out/msxdoom.ihx out/crt0.rel out/main.rel
makebin -s 32768 out/msxdoom.ihx out/full.bin
dd if=out/full.bin of=out/msxdoom.rom bs=16384 skip=1 count=1 2>/dev/null
ls -l out/msxdoom.rom
