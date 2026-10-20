set throttle off
proc tap {row mask} { keymatrixdown $row $mask; after time 0.15 [list keymatrixup $row $mask] }
proc snap {n} { save_debuggable VRAM $::env(SHOTDIR)/$n.vram }
after time 8 { tap 8 0x01 }
after time 19 { tap 8 0x01 }
after time 23 { snap u0 }
after time 24 { tap 8 0x01 }
after time 26 { tap 8 0x01 }
after time 28 { tap 8 0x01 }
after time 31 { snap u1 }
after time 32 { exit }
