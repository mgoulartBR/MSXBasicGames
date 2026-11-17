#!/bin/bash
declare -A W=([1]="688 624" [2]="608 448" [3]="848 848" [4]="256 880" [5]="1264 704" [6]="160 608" [7]="208 384" [8]="48 352" [9]="928 512" [10]="1216 512" [11]="720 176" [12]="720 224")
: > walkall.out
for L in 1 2 3 4 5 6 7 8 9 10 11 12; do
 for v in vb975 vb; do
  r=$(BENCH_Z80=5369318 BENCH_SECS=30 BENCH_LEG=1.5 timeout 280 ./runp.sh ../$v wa_${v}_$L $L "${W[$L]}" 2>&1 | grep -E "fps=|lives=" | tr '\n' ' ')
  echo "L$L $v $r" >> walkall.out
 done
done
echo DONE >> walkall.out
