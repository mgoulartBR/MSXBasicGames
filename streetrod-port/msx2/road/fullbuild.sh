#!/bin/sh
# build completo com os dados originais (ajuste SR para a pasta com LIB2, HOT_DATA e SRSE.EXE)
SR=${SR:-/tmp/claude-0/-home-user-MSXBasicGames/fcd63da7-a365-57b2-8612-d6eea0217e5d/scratchpad/sr}
export LIB2=$SR/LIB2 HOT_DATA=$SR/HOT_DATA SRSE_EXE=$SR/SRSE.EXE
cd "$(dirname "$0")" && ./build.sh "$@"
