#!/bin/sh
# BUILD PRIVADO do E1M1 (para uso pessoal): requer SDCC (>=4.2), Python 3 + numpy + pillow + gcc, e o SEU doom1.wad.
# uso: tools/build_e1m1.sh /caminho/doom1.wad [skill 1-5]   ->  private/e1m1.rom (NUNCA versione o WAD, os dados convertidos nem a ROM)
set -e
cd "$(dirname "$0")/.."; mkdir -p out private
WAD="$1"; SKILL="${2:-3}"
[ -f "$WAD" ] || { echo "uso: $0 doom1.wad [skill]"; exit 1; }
python3 tools/e1m1_build.py "$WAD" "$SKILL"
sdasz80 -plosgff -o out/crt0.rel src/crt0.s
sdasz80 -plosgff -o out/math.rel src/math.s
sdcc -mz80 --opt-code-size -Iprivate -c -o out/e1m1.rel src/e1m1.c
sdcc -mz80 --no-std-crt0 --code-loc 0x8000 --data-loc 0xC000 -o out/e1m1.ihx out/crt0.rel out/math.rel out/e1m1.rel
python3 tools/mkrom.py out/e1m1.ihx private/e1m1_banks.bin private/e1m1.rom
