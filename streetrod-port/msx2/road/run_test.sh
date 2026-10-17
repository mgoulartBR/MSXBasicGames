#!/bin/sh
# uso: ./run_test.sh [rom] [screenshot.png]
cd "$(dirname "$0")"
ROM=${1:-road.rom}; SHOT=${2:-/tmp/road.png}
export SHOT SDL_AUDIODRIVER=dummy
xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart "$ROM" -script measure.tcl 2>&1 | grep -E "RESULT|rror|arning" | head -20
