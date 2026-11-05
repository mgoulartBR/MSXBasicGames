@echo off
cd /d "%~dp0"
openmsx -machine C-BIOS_MSX2+ -cart KAW9968.ROM -romtype ASCII8 -script v9968_shim.tcl
