#!/bin/bash
# Run the ROM in openMSX (interactive window). Mouse is plugged in port A; joystick in port B is free.
source "$(dirname "$0")/env.sh"
ROM="${1:-$PROJ_DIR/dist/$ROM_NAME-msx-$VERSION.rom}"
exec openmsx -machine "${MACHINE:-C-BIOS_MSX2}" -cart "$ROM" -romtype ASCII8 -command "plug joyporta mouse"
