source helpers011.tcl
proc rp_case {} {
 delay 45
 delay 3;shot rnprobe-start
 puts stderr "INFO ticks=[w sim_ticks] sprites_dirty=[b sprites_dirty] unit_count=[b unit_count] hp=[b unit_hp 0],[b unit_hp 1],[b unit_hp 2],[b unit_hp 3] hp8=[b unit_hp 8],[b unit_hp 9],[b unit_hp 10],[b unit_hp 11] vis=[b actor_visible 0],[b actor_visible 1],[b actor_visible 2],[b actor_visible 3] rend=[w rendered_frames]"
 puts stderr "RESULT failures=0";exit
}
coroutine rp_run guard rp_case
after realtime 300 {puts stderr "FAIL wd";exit}
