#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
exec openmsx -machine C-BIOS_MSX2+ -cart KAW68PV.ROM -romtype ASCII8 -script play-preview-v9958.tcl
