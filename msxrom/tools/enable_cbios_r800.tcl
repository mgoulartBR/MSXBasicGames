# Somente para o perfil sintetico C-BIOS + S1990: C-BIOS MSX2+ nao tem CHGCPU.
# Nao usar como substituto da BIOS num turbo R real.
set e1m1_rom [file join [file dirname [file dirname [info script]]] private e1m1.rom]
if {[info exists env(E1M1_ROM)]} {set e1m1_rom $env(E1M1_ROM)}
set e1m1_file [open $e1m1_rom rb]
binary scan [read $e1m1_file 4] c4 e1m1_header
close $e1m1_file
set e1m1_entry [expr {([lindex $e1m1_header 2]&255)+256*([lindex $e1m1_header 3]&255)}]
debug set_bp -once $e1m1_entry {[debug read memory 0x4000]==65 && [debug read memory 0x4001]==66} {debug write {S1990 regs} 6 64}
