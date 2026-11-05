source helpers.tcl
proc native_missing {} {
 delay 15
 ck rejects_v9958 {[b chip_id]!=3 && [b chip_id]!=4}
 ck renderer_not_started {[b video_ready]==0 && [w loops]==0}
 ck keyclick_zero {[debug read memory 0xF3DB]==0}
 set ::throttle on
 after realtime 0.5 [info coroutine];yield
 after frame [info coroutine];yield
 screenshot -raw -doublesize ../screenshots/V9968_detection_no_device.png
 puts stderr "RESULT failures=$::failures";exit
}
coroutine native native_missing
after realtime 20 {puts stderr "FAIL watchdog";exit}
