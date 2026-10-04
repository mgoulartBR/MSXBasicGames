#!/bin/bash
# PC-sampling profiler: scripts/profile.sh [tcl script]   (default tests/prof.tcl: briefing screen)
source "$(dirname "$0")/env.sh"
cd "$RT_ROOT"
./scripts/shot.sh "${1:-tests/prof.tcl}" > /dev/null 2>&1
python3 tools/symbolize.py /tmp/rt_prof.txt out/republia.map
