#!/bin/bash
# Mouse tests.
#  1. DBG_MOUSE_TEST ROM: hardware polling is replaced by pointer state poked from Tcl (openMSX cannot
#     inject mouse motion): hover, size select, pick up, footprint, drop, re-pick, right-click discard.
#  2. Release ROM with an emulated MSX mouse plugged (at rest): detection must not produce phantom input.
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
RT_TAG=mouse RT_DEFINES="-DDBG_MOUSE_TEST" ./scripts/build.sh >/dev/null 2>&1 || { echo "mouse build failed"; exit 1; }
cp out/republia.map build/mouse.map
RT_MAP=build/mouse.map ./scripts/shot.sh tests/mouse.tcl C-BIOS_MSX1_EU "dist/republia-msx-$VERSION-mouse.rom" >/dev/null 2>&1
RT_TAG=mousetxt RT_DEFINES="-DDBG_MOUSE_TEST -DDBG_FAST=10" ./scripts/build.sh >/dev/null 2>&1 && cp out/republia.map build/mousetxt.map
RT_MAP=build/mousetxt.map ./scripts/shot.sh tests/mouse_text.tcl C-BIOS_MSX1_EU "dist/republia-msx-$VERSION-mousetxt.rom" >/dev/null 2>&1
./scripts/build.sh >/dev/null 2>&1
cp out/republia.map build/rel.map
RT_MAP=build/rel.map RT_OUT=build/plug.txt ./scripts/shot.sh tests/mouse_plug.tcl >/dev/null 2>&1
cat build/plug.txt
grep -q "play on=0" build/plug.txt && echo "mouse plug test: ok" || { echo "mouse plug test: FAILED"; exit 1; }
