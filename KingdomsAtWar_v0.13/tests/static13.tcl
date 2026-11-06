source kingdom_helpers.tcl
proc st13 {} {delay 45;choose 2;delay 2;shot static13;choose 3;delay 2;shot static13b;puts stderr "RESULT failures=0";exit}
coroutine st13_case st13
after realtime 100 {puts stderr "FAIL";exit}
