# boot, wait, screenshot
set out $::env(SHOT_OUT)
after time 5 "screenshot $out/boot.png; exit"
