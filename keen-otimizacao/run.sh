#!/bin/bash
# uso: run.sh <projeto> <rotulo> [maquina]
P=$(realpath $1); L=$2; M=${3:-C-BIOS_MSX2+}; B=/tmp/claude-0/-home-user-MSXBasicGames/cebb32c5-1abc-529f-a253-d0e39cbf9a57/scratchpad/bench
cd $P && python3 $B/mkbench.py . $B/$L.tcl && cd $B && rm -f $L.txt
export KEEN_WORLD_TEST=1 KEEN_TEST_OUTPUT=$B BENCH_OUT=$B/$L.txt BENCH_SHOT=$B/$L
timeout 200 xvfb-run -a env SDL_AUDIODRIVER=dummy openmsx -machine $M -cart $P/build/keen_msx.rom -romtype ASCII16 -script $B/$L.tcl >$B/$L.log 2>&1
cat $B/$L.txt
