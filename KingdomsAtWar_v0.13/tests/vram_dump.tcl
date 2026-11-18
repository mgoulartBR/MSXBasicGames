# Despeja as regioes de VRAM escritas na carga (poses/oclusao 0x6000..0x38000, padroes da camada B, tiles inimigos crus) em arquivos
# para comparar duas ROMs byte a byte. Saida: $KAW_DUMP_DIR/vram_*.bin
source kingdom_helpers.tcl
proc vd_case {} {
 delay 75
 set dir $::env(KAW_DUMP_DIR)
 foreach {name a n} {poses 0x6000 0x32000 patterns 0x40000 0x14000 enemyraw 0x60000 0x6000} {
  set f [open $dir/vram_$name.bin wb];fconfigure $f -translation binary
  puts -nonewline $f [debug read_block {Sunrise GFX9000 VRAM} $a $n];close $f
 }
 puts stderr "RESULT failures=0";exit
}
coroutine vd_run guard vd_case
after realtime 400 {puts stderr "FAIL vram_dump watchdog";exit}
