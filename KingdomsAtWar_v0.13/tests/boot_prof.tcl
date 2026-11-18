# Perfil de PC do boot (amostra a cada 5 ms emulados) ate o jogo comecar: onde vai o tempo de carga.
source kingdom_helpers.tcl
source ranges_V9990P1.tcl
set ::bp_samples [dict create];set ::bp_n 0
proc bp_sample {} {
 set pc [reg PC];set bank [expr {$pc<40960?0:[debug read memory 0xC006]}];set key $bank:unknown
 if {[info exists ::ranges($bank)]} {foreach r $::ranges($bank) {lassign $r a n;if {$a>$pc} {break};set key $bank:$n}}
 dict incr ::bp_samples $key;incr ::bp_n
 after time 0.005 bp_sample
}
proc bp_case {} {
 bp_sample
 delay 46
 set a {};dict for {k v} $::bp_samples {lappend a [list $v $k]}
 puts stderr "BOOTPROF total $::bp_n"
 foreach e [lrange [lsort -decreasing -integer -index 0 $a] 0 29] {puts stderr "BOOTPROF [format %5.1f [expr {100.0*[lindex $e 0]/$::bp_n}]]% [lindex $e 0] [lindex $e 1]"}
 puts stderr "RESULT failures=0";exit
}
coroutine bp_run guard bp_case
after realtime 400 {puts stderr "FAIL boot_prof watchdog";exit}
