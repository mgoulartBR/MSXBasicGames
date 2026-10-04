#!/bin/bash
# Run the ROM in openMSX (interactive). Usage: scripts/run.sh [machine]   (default C-BIOS_MSX1_EU, 50 Hz)
source "$(dirname "$0")/env.sh"
MACHINE="${1:-C-BIOS_MSX1_EU}"
ROM="${2:-$RT_ROOT/dist/republia-msx-$VERSION.rom}"
exec openmsx -machine "$MACHINE" -cart "$ROM" -romtype ASCII8
