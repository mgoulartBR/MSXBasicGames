# Assertions on the game state through the RAM beacon of the debug ROM (see ui_menu.c / beacon()).
set ::RES [open $::env(RESULTS) a]
set ::BEACON $::env(BEACON)
proc bk {i} { return [peek [expr {$::BEACON + $i}]] }
proc bk16 {i} { return [expr {[bk $i] | ([bk [expr {$i+1}]] << 8)}] }
proc bk32 {i} { return [expr {[bk $i] | ([bk [expr {$i+1}]] << 8) | ([bk [expr {$i+2}]] << 16) | ([bk [expr {$i+3}]] << 24)}] }
proc check {name cond} {
    if {[uplevel 1 [list expr $cond]]} { puts $::RES "PASS $name" } else { puts $::RES "FAIL $name ([uplevel 1 [list subst $cond]])" }
    flush $::RES
}
# beacon indexes
set ::B(screen) 0; set ::B(phase) 1; set ::B(ante) 2; set ::B(blind) 3; set ::B(money) 4; set ::B(hands) 6; set ::B(discards) 7
set ::B(nhand) 8; set ::B(njk) 9; set ::B(state) 10; set ::B(score) 11; set ::B(sel) 16; set ::B(pile) 17; set ::B(mouse) 18
# screens
set ::SC(title) 0; set ::SC(blind) 1; set ::SC(round) 2; set ::SC(cashout) 3; set ::SC(shop) 4; set ::SC(pack) 5; set ::SC(info) 6; set ::SC(over) 7; set ::SC(win) 8
proc at_check {t script} { at $t $script }
