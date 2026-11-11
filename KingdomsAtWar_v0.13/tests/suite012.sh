#!/usr/bin/env bash
set -euo pipefail
KAW_SUITE_VARIANT="$1"
python3 tests/check_memory092.py "$KAW_SUITE_VARIANT"
for KAW_SUITE_CASE in kernel012 perf012 fog011 combat011 worker011 camera011 overlay011 perf091; do
 KAW_TEST_SCRIPT="$KAW_SUITE_CASE.tcl" python3 tests/run_tests.py "$KAW_SUITE_VARIANT"
done
if [[ "$KAW_SUITE_VARIANT" == V9990 || "$KAW_SUITE_VARIANT" == V9990P1 ]]; then KAW_SUITE_CASE=defense011;else KAW_SUITE_CASE=kingdom011;fi
KAW_TEST_SCRIPT="$KAW_SUITE_CASE.tcl" python3 tests/run_tests.py "$KAW_SUITE_VARIANT"
python3 tests/run_mouse.py "$KAW_SUITE_VARIANT" box011.tcl 1
if [[ "$KAW_SUITE_VARIANT" == V9990P1 ]]; then
 for KAW_P1_CASE in occl_p1 flash_p1 flashseq_p1 mine_crop_p1 enemy_kinds defend011; do KAW_TEST_SCRIPT="$KAW_P1_CASE.tcl" python3 tests/run_tests.py V9990P1; done
 python3 tests/verify_fog_p1.py
fi
