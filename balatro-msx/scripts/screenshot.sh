#!/bin/bash
# Boot the ROM headless (Xvfb), run an optional openMSX TCL script, save screenshots.
# Usage: scripts/screenshot.sh <tcl-script> [rom]
source "$(dirname "$0")/env.sh"
TCL="$1"; ROM="${2:-$PROJ_DIR/dist/$ROM_NAME-msx-$VERSION.rom}"
timeout 180 xvfb-run -a -s "-screen 0 1024x768x24" openmsx -machine "${MACHINE:-C-BIOS_MSX2}" -romtype ASCII8 -cart "$ROM" \
  -command "plug joyporta mouse" -script "$TCL" 2>&1 | grep -vE "ALSA|audio|sequencer|SRAM"
