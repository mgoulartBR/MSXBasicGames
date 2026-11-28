source helpers011.tcl
proc mp_case {} {
 delay 38
 puts stderr "DEBUGGABLES [debug list]"
 puts stderr "INFO fm_ok=[b fm_ok] cur=[b music_cur] ms_on=[b ms 0]"
 exit
}
coroutine mp_run guard mp_case
after realtime 200 {exit}
