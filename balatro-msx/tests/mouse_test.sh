#!/bin/bash
# Mouse smoke test: boots the ROM in openMSX on a private Xvfb display, moves/clicks the *X11 pointer* with xdotool
# (openMSX translates it into MSX mouse movement on port A) and takes screenshots through the openMSX console.
# Usage: tests/mouse_test.sh [rom]
source "$(dirname "$0")/../scripts/env.sh"
ROM="${1:-$PROJ_DIR/dist/$ROM_NAME-msx-$VERSION-debug.rom}"
OUT="$PROJ_DIR/screenshots"; mkdir -p "$OUT"
D=:97
Xvfb $D -screen 0 1024x768x24 >/dev/null 2>&1 & XPID=$!
sleep 1
export DISPLAY=$D SHOT_OUT="$OUT"
cat > /tmp/mouse_test.tcl <<'T'
source $::env(TCL_LIB)
set b 7.0
snap [expr {$b + 1.0}] 20_mouse_title
snap [expr {$b + 10.0}] 21_mouse_round
snap [expr {$b + 13.5}] 22_mouse_selected
snap [expr {$b + 21.0}] 23_mouse_played
finish [expr {$b + 22}]
T
export TCL_LIB="$PROJ_DIR/tests/tcl/lib.tcl"
openmsx -machine C-BIOS_MSX2 -cart "$ROM" -romtype ASCII8 -command "plug joyporta mouse" -script /tmp/mouse_test.tcl >/tmp/mouse_test.log 2>&1 &
OPID=$!
sleep 3
WID=$(xdotool search --name openMSX | head -1)
echo "openMSX window: $WID"
xdotool mousemove 512 384
click() { xdotool mousedown 1; sleep 0.25; xdotool mouseup 1; sleep 0.25; }
move() { for i in $(seq 1 $3); do xdotool mousemove_relative -- $1 $2; sleep 0.05; done; sleep 0.2; }
sleep 7                          # emulated boot (C-BIOS logo) ~ real time
click                            # title -> blind select
sleep 1.5
move 0 6 4                       # onto the Select button (openMSX scales host px to roughly 0.5-0.6 MSX counts)
click                            # select the blind -> round
sleep 2.5
move 0 -6 4                      # up onto the hand
click                            # select a card
move 8 0 3
click                            # select another card
sleep 0.5
cp /dev/null /tmp/mouse_step1
move 0 14 5                      # down onto the Play button
click                            # play the hand
sleep 6
wait $OPID
kill $XPID 2>/dev/null
tail -3 /tmp/mouse_test.log
