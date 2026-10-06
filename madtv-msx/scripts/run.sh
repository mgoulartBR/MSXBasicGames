#!/bin/bash
# Executa a ROM no openMSX (C-BIOS MSX2).  Uso: scripts/run.sh [rom] [--shot arquivo.png] [--wait segundos]
# Sem --shot abre janela normal; com --shot roda headless (Xvfb), captura e sai.
cd "$(dirname "$0")/.."
ROM=${1:-dist/madtv-msx-$(cat VERSION).rom}; shift || true
SHOT=""; WAIT=6
while [ $# -gt 0 ]; do case "$1" in --shot) SHOT=$2; shift 2;; --wait) WAIT=$2; shift 2;; *) shift;; esac; done
MACHINE=${MSX_MACHINE:-C-BIOS_MSX2}
if [ -z "$SHOT" ]; then exec openmsx -machine "$MACHINE" -cart "$ROM"; fi
mkdir -p "$(dirname "$SHOT")"
SHOT_ABS=$(realpath -m "$SHOT")
cat > /tmp/madtv_run.tcl <<T
after time $WAIT { screenshot -raw "$SHOT_ABS"; after time 0.5 { exit } }
T
xvfb-run -a openmsx -machine "$MACHINE" -cart "$ROM" -script /tmp/madtv_run.tcl >/tmp/madtv_openmsx.log 2>&1
tail -3 /tmp/madtv_openmsx.log; ls -l "$SHOT_ABS"
