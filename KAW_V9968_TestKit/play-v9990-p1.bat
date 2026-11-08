@echo off
cd /d "%~dp0"
openmsx -setting kaw_openmsx_settings.xml -machine C-BIOS_MSX2 -ext gfx9000 -cart KAW9990P1.ROM -romtype ASCII8 -script play-v9990.tcl
