#!/bin/bash
# Headless openMSX run driven by a Tcl script; usage: scripts/shot.sh <script.tcl> [machine]
# The Tcl script takes screenshots into screenshots/ and ends with "exit".
source "$(dirname "$0")/env.sh"
MACHINE="${2:-C-BIOS_MSX1_EU}"
ROM="${3:-$RT_ROOT/dist/republia-msx-$VERSION.rom}"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 300 openmsx -machine "$MACHINE" \
  -cart "$ROM" -romtype ASCII8 -script "$1" 2>&1 | grep -v -E "ALSA|sequencer|audio|OpenGL|renderer|SDL renderer" 
