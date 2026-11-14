#!/bin/bash
# uso: shots.sh <projeto> <rotulo> <nivel> "<worldx> <worldy>" "<lista de cenas tcl>"
P=$(realpath $1); L=$2; B=/tmp/claude-0/-home-user-MSXBasicGames/cebb32c5-1abc-529f-a253-d0e39cbf9a57/scratchpad/bench
mkdir -p $B/shots/$L; rm -f $B/shots/$L/*
cd $P && python3 $B/mkshots.py . $B/shots_$L.tcl && cd $B
export KEEN_WORLD_TEST=1 KEEN_TEST_OUTPUT=$B SHOT_DIR=$B/shots/$L SHOT_LEVEL=$3 SHOT_WORLD="$4" SHOT_SCENES="$5"
timeout 250 xvfb-run -a env SDL_AUDIODRIVER=dummy openmsx -machine C-BIOS_MSX2+ -cart $P/build/keen_msx.rom -romtype ASCII16 -script $B/shots_$L.tcl >$B/shots_$L.log 2>&1
ls $B/shots/$L | head -20
