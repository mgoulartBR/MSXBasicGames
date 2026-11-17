#!/bin/bash
declare -A W=([1]="688 624" [2]="608 448" [3]="848 848" [4]="256 880" [5]="1264 704" [6]="160 608" [7]="208 384" [8]="48 352" [9]="928 512" [10]="1216 512" [11]="720 176" [12]="720 224")
: > trackall.done
for L in 1 2 3 4 5 6 7 8 9 10 11 12; do
 rm -f trk_$L.*
 BENCH_GOD=1 BENCH_TRACK=2 BENCH_JUMPT=0.5 BENCH_LEG=8 BENCH_Z80=7159090 BENCH_SECS=90 timeout 290 ./runp.sh ../vb975 trk_$L $L "${W[$L]}" >/dev/null 2>&1
 echo "L$L $(wc -l < trk_$L.txt.trk 2>/dev/null)" >> trackall.done
done
echo DONE >> trackall.done
