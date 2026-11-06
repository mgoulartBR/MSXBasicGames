plug joyporta mouse
set videosource GFX9000
# O openMSX grava z80_freq nas configuracoes: este kit usa um arquivo proprio (kaw_openmsx_settings.xml) e fixa o clock sempre.
if {[info exists ::env(KAW_TURBO)]} {set z80_freq_locked false;set z80_freq 10738635} else {set z80_freq_locked false;set z80_freq 3579545;set z80_freq_locked true}
