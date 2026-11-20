#!/bin/bash
declare -A W=([1]="688 624" [2]="608 448" [3]="848 848" [4]="256 880" [5]="1264 704" [6]="160 608" [7]="208 384" [8]="48 352" [9]="928 512" [10]="1216 512" [11]="720 176" [12]="720 224")
: > walkfin.out
for L in 1 2 3 4 5 6 7 8 9 10 11 12; do
 for spec in "vb975 5369318" "vb 5369318" "vb 7159090" "vb 3579545"; do set -- $spec
  r=$(BENCH_Z80=$2 BENCH_SECS=40 BENCH_GOD=2 ./runp.sh ../$1 wf_${1}_$2_$L $L "${W[$L]}" 2>&1 | grep -oE "fps=[0-9.]+" | head -1)
  echo "L$L $1 $2 $r" >> walkfin.out
 done
done
echo DONE >> walkfin.out
