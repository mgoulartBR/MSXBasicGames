#!/bin/bash
# uso: runp.sh <projeto> <rotulo> <nivel> "<wx> <wy>" [maquina] ; env: BENCH_PLACE="x y" BENCH_SECS BENCH_LEG
P=$(realpath $1); L=$2; B=$(pwd)
cd $P && python3 $B/mkprof.py . $B/$L.tcl && cd $B && rm -f $L.txt
export KEEN_WORLD_TEST=1 KEEN_TEST_OUTPUT=$B BENCH_OUT=$B/$L.txt BENCH_LEVEL=$3 BENCH_WORLD="$4" BENCH_LOADWAIT=${BENCH_LOADWAIT:-20} BENCH_SECS=${BENCH_SECS:-20} BENCH_LEG=${BENCH_LEG:-1.5} BENCH_SHOT=$B/$L.png
timeout 290 xvfb-run -a env SDL_AUDIODRIVER=dummy openmsx -machine ${5:-C-BIOS_MSX2+} -cart $P/build/keen_msx.rom -romtype ASCII16 -script $B/$L.tcl >$B/$L.log 2>&1
cat $B/$L.txt
