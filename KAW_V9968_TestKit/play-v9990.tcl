plug joyporta mouse
set videosource GFX9000
# TURBO: defina KAW_TURBO=1 (ou use play-v9990-turbo.*) para simular Z80 acelerado (10,7 MHz, como MSXBook/turbo)
if {[info exists ::env(KAW_TURBO)]} {set z80_freq_locked false;set z80_freq 10738635}
