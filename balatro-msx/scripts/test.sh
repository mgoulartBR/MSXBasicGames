#!/bin/bash
# Full test run: host unit tests + simulation, Z80 self-test ROM, openMSX smoke test (headless, C-BIOS).
# Usage: scripts/test.sh [quick]     quick = host tests only
source "$(dirname "$0")/env.sh"
cd "$PROJ_DIR" || exit 1
FAIL=0
step() { echo; echo "== $* =="; }
mkdir -p build screenshots
step "host tests (shared cases + random-run simulation, ASan/UBSan)"
$PY tools/gen_music.py >/dev/null; $PY tools/gen_data.py >/dev/null && $PY tools/gen_assets.py >/dev/null
CF="-Wall -Wextra -O1 -g -fsanitize=address,undefined -I include -I src"
gcc $CF -o build/host_cases tests/host_main.c tests/test_cases.c tests/test_cases2.c src/game/*.c src/seg/seg_s20_b3.c || FAIL=1
gcc $CF -o build/host_sim tests/host_sim.c src/game/*.c src/seg/seg_s20_b3.c || FAIL=1
./build/host_cases | tail -2 || FAIL=1
./build/host_sim 300 | tail -4 || FAIL=1
[ "$1" = "quick" ] && exit $FAIL
export SHOT_OUT="$PROJ_DIR/screenshots" RESULTS="$PROJ_DIR/build/smoke_results.txt"
rm -f "$RESULTS"
step "build release / debug / self-test ROMs"
bash scripts/build.sh >/dev/null || FAIL=1
bash scripts/build.sh selftest >/dev/null || FAIL=1
ST=$(grep "_g_selftest" out/balatro.map | head -1 | awk '{print "0x"$1}')
bash scripts/build.sh debug >/dev/null || FAIL=1
BEACON=$(grep "_g_beacon" out/balatro.map | head -1 | awk '{print "0x"$1}')
PERF=$(grep "_g_perf" out/balatro.map | head -1 | awk '{print "0x"$1}')
export ST BEACON PERF
step "Z80 self-test in openMSX"
bash scripts/screenshot.sh tests/tcl/selftest.tcl "dist/$ROM_NAME-msx-$VERSION-selftest.rom" >/dev/null 2>&1
step "openMSX smoke test (C-BIOS MSX2, mouse in port A)"
bash scripts/screenshot.sh tests/tcl/smoke_all.tcl "dist/$ROM_NAME-msx-$VERSION-debug.rom" >/dev/null 2>&1
for extra in smoke_mods smoke_jokers; do
  bash scripts/screenshot.sh tests/tcl/$extra.tcl "dist/$ROM_NAME-msx-$VERSION-debug.rom" >/dev/null 2>&1
done
cat "$RESULTS"
grep -q FAIL "$RESULTS" && FAIL=1
[ "$(grep -c PASS "$RESULTS")" -ge 15 ] || { echo "too few checks passed"; FAIL=1; }
echo; [ $FAIL = 0 ] && echo "ALL TESTS PASSED" || echo "TESTS FAILED"
exit $FAIL
