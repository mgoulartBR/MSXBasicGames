#!/usr/bin/env bash
set -euo pipefail
KAW_SUITE_VARIANT="$1"
python3 tests/check_memory092.py "$KAW_SUITE_VARIANT"
for KAW_SUITE_CASE in perf012 fog011 combat011 worker011 camera011 overlay011 perf091; do
 KAW_TEST_SCRIPT="$KAW_SUITE_CASE.tcl" python3 tests/run_tests.py "$KAW_SUITE_VARIANT"
done
if [[ "$KAW_SUITE_VARIANT" == V9990 || "$KAW_SUITE_VARIANT" == V9990P1 ]]; then KAW_SUITE_CASE=defense011;else KAW_SUITE_CASE=kingdom011;fi
KAW_TEST_SCRIPT="$KAW_SUITE_CASE.tcl" python3 tests/run_tests.py "$KAW_SUITE_VARIANT"
for _try in 1 2 3; do python3 tests/run_mouse.py "$KAW_SUITE_VARIANT" box011.tcl 1 && break; [[ $_try == 3 ]] && exit 1; done
if [[ "$KAW_SUITE_VARIANT" == V9990P1 ]]; then
 for KAW_P1_CASE in occl_p1 flash_p1 flashseq_p1 mine_crop_p1 enemy_kinds defend011 enemy_tower endgame enemy_visual enemy_site human_site enemy_extras; do KAW_TEST_SCRIPT="$KAW_P1_CASE.tcl" python3 tests/run_tests.py V9990P1; done
 KAW_MENU=1 KAW_TEST_SCRIPT=menu_flow.tcl python3 tests/run_tests.py V9990P1
 KAW_MENU=1 KAW_OBJ=1 KAW_TEST_SCRIPT=campaign_flow.tcl python3 tests/run_tests.py V9990P1
 for KAW_BIG in bigmap bigmap_ai bigmap128 allmaps; do KAW_MENU=1 KAW_OBJ=1 KAW_TEST_SCRIPT=$KAW_BIG.tcl python3 tests/run_tests.py V9990P1; done
 KAW_SIDE=1 KAW_MENU=1 KAW_OBJ=1 KAW_TEST_SCRIPT=allmaps.tcl python3 tests/run_tests.py V9990P1
 KAW_MENU=1 KAW_TEST_SCRIPT=undead_campaign.tcl python3 tests/run_tests.py V9990P1
 python3 tests/run_mouse.py V9990P1 edge_bottom.tcl 1
 KAW_TEST_SCRIPT=stale_heal.tcl python3 tests/run_tests.py V9990P1
 KAW_MENU=1 KAW_OBJ=1 KAW_MACHINE=C-BIOS_MSX2+ KAW_TEST_SCRIPT=music.tcl python3 tests/run_tests.py V9990P1
 KAW_MENU=1 KAW_OBJ=1 KAW_MACHINE=C-BIOS_MSX2+ KAW_TEST_SCRIPT=ui_polish.tcl python3 tests/run_tests.py V9990P1
 KAW_MENU=1 KAW_OBJ=1 KAW_TEST_SCRIPT=ui_hud.tcl python3 tests/run_tests.py V9990P1
 KAW_MENU=1 KAW_MACHINE=C-BIOS_MSX2+ KAW_TEST_SCRIPT=options_menu.tcl python3 tests/run_tests.py V9990P1
 python3 tests/verify_fog_p1.py
fi
