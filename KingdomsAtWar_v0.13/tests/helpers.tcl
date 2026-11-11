# Real ROM execution: keyboard inputs and read-only RAM/VDP assertions.
set throttle off
# openMSX PERSISTE z80_freq entre execucoes: sempre fixa o clock explicitamente (3,58 MHz salvo KAW_Z80FREQ).
if {[info exists ::env(KAW_Z80FREQ)]} {set z80_freq_locked false;set z80_freq $::env(KAW_Z80FREQ)} else {set z80_freq_locked false;set z80_freq 3579545;set z80_freq_locked true}
set maxframeskip 0
if {![info exists ::env(KAW_NO_MOUSE)]} {if {[info exists ::env(KAW_MOUSE_PORT)] && $::env(KAW_MOUSE_PORT)==2} {plug joyportb mouse} else {plug joyporta mouse}}
source symbols-$::env(KAW_VARIANT).tcl
if {$::env(KAW_VARIANT) eq "V9968"} {source v9968_shim.tcl}
if {$::env(KAW_VARIANT) eq "V9990" || $::env(KAW_VARIANT) eq "V9990P1"} {set videosource GFX9000}
set failures 0
# Os testes antigos assumem a base inimiga vazia: desliga a IA inimiga (ai_enabled) logo apos a inicializacao, salvo KAW_AI=1.
if {![info exists ::env(KAW_AI)]} {after time 9 {debug write memory $::S(ai_enabled) 0}}
# Idem para a defesa automatica dos soldados parados (auto_defend), salvo KAW_DEF=1.
if {![info exists ::env(KAW_DEF)]} {after time 9 {debug write memory $::S(auto_defend) 0}}
proc b {name {off 0}} {return [debug read memory [expr {$::S($name)+$off}]]}
proc w {name} {return [expr {[b $name]+256*[b $name 1]}]}
proc ck {label expression} {
 if {[uplevel 1 [list expr $expression]]} {puts stderr "PASS $label"} else {puts stderr "FAIL $label"; incr ::failures}
}
proc shot {name} {
 delay 0.12
 # Let the renderer present current VRAM; fast-forward can retain an old frame.
 set ::throttle on
 set ::speed 100
 after realtime 0.6 [info coroutine]
 yield
 after frame [info coroutine]
 yield
 screenshot -raw -doublesize ../screenshots/$::env(KAW_VARIANT)/$name.png
 set ::throttle off
}
proc unit {id field} {
 set offsets [dict create px 0 py 2 x 4 y 5 tx 6 ty 7 nx 8 ny 9 owner 10 len 11 pos 12 progress 13 phase 14 state 15]
 set off [expr {16*$id+[dict get $offsets $field]}]
 if {$field eq "px" || $field eq "py"} {return [expr {[b units $off]+256*[b units [expr {$off+1}]]}]}
 return [b units $off]
}
proc state {} {puts stderr "STATE camera=[w cam_x],[w cam_y] unit0=[unit 0 px],[unit 0 py] state=[unit 0 state] queue=[w qhead]/[w qtail] ticks=[w sim_ticks] scrolls=[w scroll_copies] edges=[w terrain_copies] loops=[w loops]"}
proc delay {seconds} {after time $seconds [info coroutine]; yield}
proc tap {row mask} {
 set start [w loops];keymatrixdown $row $mask;delay 0.12
 await {(([w loops]-$start)&65535)>=2} 8
 keymatrixup $row $mask;set start [w loops];delay 0.12
 await {(([w loops]-$start)&65535)>=2} 8
}
proc point {tx ty} {
 foreach {name target row neg pos} [list mouse_x $tx 4 0 2 mouse_y $ty 4 0 1] {
  # Negative X uses J (row3 bit7); negative Y uses I (row3 bit6).
  set start [b $name]; set positive [expr {$start<$target}]
  if {abs($start-$target)<=2} continue
  if {$positive} {set r $row; set m $pos} else {set r 3; set m [expr {$name eq "mouse_x"?128:64}]}
  keymatrixdown $r $m
  set count 0
  while {($positive && [b $name]<$target) || (!$positive && [b $name]>$target)} {
   delay 0.016667
   if {[incr count]>6000} {error "cursor timeout target=$target axis=$name value=[b $name] cam=[w cam_x],[w cam_y] loops=[w loops] PC=[reg PC] place=[b placement_mode]"}
  }
  keymatrixup $r $m
  delay 0.05
 }
}
proc await {expression {limit 20}} {
 set t [machine_info time]
 while {![uplevel 1 [list expr $expression]]} {
  delay 0.033333
  if {[machine_info time]-$t>$limit} {error "timeout: $expression"}
 }
}
proc camera_until {name target mask} {
 keymatrixdown 8 $mask
 set start [w $name]; set rising [expr {$target>$start}]
 if {$rising} {await {[w $name]>=$target} 80} else {await {[w $name]<=$target} 80}
 keymatrixup 8 $mask
 delay 0.1
}

