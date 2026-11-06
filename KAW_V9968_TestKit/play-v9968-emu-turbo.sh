#!/usr/bin/env bash
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
KAW_TURBO=1 exec openmsx -machine C-BIOS_MSX2+ -cart KAW9968.ROM -romtype ASCII8 -script v9968_shim.tcl
