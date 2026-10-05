# Tiny scripting helpers for openMSX smoke tests (emulated-time timeline).
set ::T 0.0
proc shot {name} { set ::SHOTS_N [expr {[info exists ::SHOTS_N] ? $::SHOTS_N : 0}]; screenshot $::env(SHOT_OUT)/$name.png }
proc at {t script} { after time $t $script }
# press a key (matrix row/bit) for 0.08s at emulated time t
proc press {t row bit} {
    set mask [expr {1 << $bit}]
    at $t "keymatrixdown $row $mask"
    at [expr {$t + 0.08}] "keymatrixup $row $mask"
}
proc key {t name} {
    switch $name {
        space {press $t 8 0}  return {press $t 7 7}  esc {press $t 7 2}
        left  {press $t 8 4}  up {press $t 8 5}  down {press $t 8 6}  right {press $t 8 7}
        6 {press $t 0 6}  7 {press $t 0 7}  1 {press $t 0 1}  2 {press $t 0 2}  3 {press $t 0 3}  4 {press $t 0 4}  5 {press $t 0 5}
        8 {press $t 1 0}  p {press $t 4 5}  d {press $t 3 1}  s {press $t 5 0}  i {press $t 3 6}  n {press $t 4 3}
    }
}
proc snap {t name} { at $t "screenshot -raw -doublesize $::env(SHOT_OUT)/$name.png" }
proc finish {t} { at $t "exit" }
