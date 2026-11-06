@echo off
cd /d "%~dp0"
set KAW_TURBO=1
openmsx -machine C-BIOS_MSX2 -ext gfx9000 -cart KAW9990.ROM -romtype ASCII8 -script play-v9990.tcl
