#!/usr/bin/env bash
# Roda uma amostra da suite em varias maquinas do openMSX (BIOS diferentes: C-BIOS MSX2/MSX2+, regioes EU/JP/BR).
# uso: tests/machines_smoke.sh   (o jogo P1 precisa de V9990; a maquina e trocada por KAW_MACHINE)
set -u
for M in ${KAW_MACHINES:-C-BIOS_MSX2 C-BIOS_MSX2_EU C-BIOS_MSX2_JP C-BIOS_MSX2_BR C-BIOS_MSX2+ C-BIOS_MSX2+_EU}; do
 echo "== $M"
 for T in perf012 camera011 combat011 flashseq_p1; do
  printf '%s ' "$T"; KAW_MACHINE=$M KAW_TEST_SCRIPT=$T.tcl python3 tests/run_tests.py V9990P1 2>&1 | grep -E "FAIL|RESULT" | sort -u | tr '\n' ' '; echo
 done
 printf 'box011 '; KAW_MACHINE=$M python3 tests/run_mouse.py V9990P1 box011.tcl 1 2>&1 | grep -E "FAIL|RESULT" | sort -u | tr '\n' ' '; echo
 printf 'fps '; KAW_MACHINE=$M python3 tests/fps_active.py V9990P1 | head -1
done
