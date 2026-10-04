#!/bin/bash
# Builds debug ROMs that start on specific days/states and captures the briefing screens.
# Verifies story branches that need many game days to reach (goal pass/fail, rebel path, endings).
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
run() { # tag defines
  RT_TAG="$1" RT_DEFINES="$2" ./scripts/build.sh >/dev/null 2>&1 || { echo "build $1 failed"; return 1; }
  RT_SHOT="m_$1" ./scripts/shot.sh tests/morning.tcl C-BIOS_MSX1_EU "dist/republia-msx-$VERSION-$1.rom" >/dev/null 2>&1
  echo "captured m_$1"
}
run d4   "-DDBG_START_DAY=4 -DDBG_LOYALTY=25 -DDBG_READERS=300"     # first task passed
run d4f  "-DDBG_START_DAY=4 -DDBG_LOYALTY=5 -DDBG_READERS=300"      # first task failed (game over)
run d6   "-DDBG_START_DAY=6 -DDBG_LOYALTY=25 -DDBG_READERS=450"     # second task passed
run d8   "-DDBG_START_DAY=8 -DDBG_LOYALTY=-12 -DDBG_READERS=500"    # rebel path
run d11w "-DDBG_START_DAY=11 -DDBG_LOYALTY=-30 -DDBG_READERS=1200"  # rebels win (swaps to Democria)
run d11l "-DDBG_START_DAY=11 -DDBG_LOYALTY=5 -DDBG_READERS=1200"    # rebels fail
./scripts/build.sh >/dev/null 2>&1   # restore the release ROM in dist/
