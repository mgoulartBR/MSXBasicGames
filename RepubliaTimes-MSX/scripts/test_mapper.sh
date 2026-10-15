#!/bin/bash
# Mapper robustness (reported on real hardware: a cart that ignores ASCII8 bank writes showed black
# images and garbage text).  Checks: ASCII8 build under ASCII8, Konami build under Konami, and a
# cart that does not switch banks (32 KB plain ROM) -> the MAPPER ERROR diagnostic screen.
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
RT_MAPPER=ROM_ASCII8 RT_TAG=ascii8 ./scripts/build.sh >/dev/null 2>&1
RT_MAPPER=ROM_KONAMI RT_TAG=konami ./scripts/build.sh >/dev/null 2>&1
head -c 32768 "dist/republia-msx-$VERSION-ascii8.rom" > build/nomapper32k.rom
RT_SHOT=40_boot_ascii8  RT_ROMTYPE=ASCII8 ./scripts/shot.sh tests/boot.tcl C-BIOS_MSX1_EU "dist/republia-msx-$VERSION-ascii8.rom" >/dev/null 2>&1
RT_SHOT=41_boot_konami  RT_ROMTYPE=Konami ./scripts/shot.sh tests/boot.tcl C-BIOS_MSX1_EU "dist/republia-msx-$VERSION-konami.rom" >/dev/null 2>&1
RT_SHOT=42_boot_nomapper RT_ROMTYPE=Normal ./scripts/shot.sh tests/boot.tcl C-BIOS_MSX1_EU build/nomapper32k.rom >/dev/null 2>&1
./scripts/build.sh >/dev/null 2>&1   # restore default (ASCII8) ROM
