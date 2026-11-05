#!/usr/bin/env bash
set -euo pipefail
KAW_SUITE_VARIANT="$1"
python3 tests/check_memory092.py "$KAW_SUITE_VARIANT"
for KAW_SUITE_CASE in kernel012 perf012 fog011 combat011 worker011 camera011 overlay011 perf091; do
 KAW_TEST_SCRIPT="$KAW_SUITE_CASE.tcl" python3 tests/run_tests.py "$KAW_SUITE_VARIANT"
done
if [[ "$KAW_SUITE_VARIANT" == V9990 ]]; then KAW_SUITE_CASE=defense011;else KAW_SUITE_CASE=kingdom011;fi
KAW_TEST_SCRIPT="$KAW_SUITE_CASE.tcl" python3 tests/run_tests.py "$KAW_SUITE_VARIANT"
python3 tests/run_mouse.py "$KAW_SUITE_VARIANT" box011.tcl 1
