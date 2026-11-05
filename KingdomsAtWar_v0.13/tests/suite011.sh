#!/usr/bin/env bash
set -euo pipefail
KAW_TEST_VARIANT="$1"
python3 tests/check_memory092.py "$KAW_TEST_VARIANT"
for KAW_CASE in fog011 combat011 worker011 camera011 overlay011 perf091; do
 KAW_NO_MOUSE=1 KAW_TEST_SCRIPT="$KAW_CASE.tcl" python3 tests/run_tests.py "$KAW_TEST_VARIANT"
done
python3 tests/run_mouse.py "$KAW_TEST_VARIANT" box011.tcl 1
