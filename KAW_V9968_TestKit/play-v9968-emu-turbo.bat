@echo off
cd /d "%~dp0"
set KAW_TURBO=1
openmsx -setting kaw_openmsx_settings.xml -machine C-BIOS_MSX2+ -cart KAW9968.ROM -romtype ASCII8 -script v9968_shim.tcl
