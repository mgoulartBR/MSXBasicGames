#!/bin/bash
declare -A W=([4]="256 880" [5]="1264 704" [6]="160 608" [9]="928 512" [11]="720 176")
: > walkshort.out
for L in 4 5 6 9 11; do
 for v in vb975 vb; do
  for secs in 6 10; do
   r=$(BENCH_Z80=5369318 BENCH_SECS=$secs BENCH_LEG=1.5 timeout 250 ./runp.sh ../$v ws_${v}_$L $L "${W[$L]}" 2>&1 | grep -E "fps=|lives=" | tr '\n' ' ')
   echo "L$L $v secs=$secs $r" >> walkshort.out
  done
 done
done
echo DONE >> walkshort.out
