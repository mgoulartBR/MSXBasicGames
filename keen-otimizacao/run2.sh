#!/bin/bash
# uso: run2.sh <projeto> <rotulo> <nivel> "<wx> <wy>"
P=$(realpath $1); L=$2; B=/tmp/claude-0/-home-user-MSXBasicGames/cebb32c5-1abc-529f-a253-d0e39cbf9a57/scratchpad/bench
cd $P && python3 $B/mkbench2.py . $B/$L.tcl && cd $B && rm -f $L.txt
export KEEN_WORLD_TEST=1 KEEN_TEST_OUTPUT=$B BENCH_OUT=$B/$L.txt BENCH_LEVEL=$3 BENCH_WORLD="$4"
timeout 280 xvfb-run -a env SDL_AUDIODRIVER=dummy openmsx -machine ${MACH:-C-BIOS_MSX2+} -cart $P/build/keen_msx.rom -romtype ASCII16 -script $B/$L.tcl >$B/$L.log 2>&1
cat $B/$L.txt
