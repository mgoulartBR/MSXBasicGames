# Ambiente de teste V9968 em openMSX (nucleo V9958).
# A KAW9968.ROM roda SEM alteracao. Unico ponto emulado: o ID do chip (S#1 bits 1-5 = 3 no V9968).
# O script localiza no ROM a rotina vp_in() (bytes 4F ED 78 C9 = ld c,a / in a,(c) / ret), e na PRIMEIRA
# chamada (deteccao) faz A=6 (ID 3 << 1); depois remove o breakpoint.
# Diferenca conhecida: a formula de R#26/R#27 do V9968 num V9958 pode deslocar o scroll fino em ate 7 px.
set kaw_rom [file join [file dirname [info script]] KAW9968.ROM]
set f [open $kaw_rom rb];set data [read $f 24576];close $f
binary scan $data H* hex
set idx [string first 4fed78c9 $hex]
while {$idx>=0 && ($idx%2)} {set idx [string first 4fed78c9 $hex [expr {$idx+1}]]}
if {$idx<0} {error "vp_in nao encontrado na ROM"}
set kaw_ret [expr {0x4000 + $idx/2 + 3}]
set kaw_bp [debug set_bp $kaw_ret {[reg C]==0x99 || [reg C]==0x89} {reg A 6;incr ::kaw_hits;debug remove_bp $::kaw_bp}]
set kaw_hits 0
plug joyporta mouse
puts "V9968 shim ativo (ret de vp_in em [format 0x%04X $kaw_ret])"
