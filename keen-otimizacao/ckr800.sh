#!/bin/bash
declare -A W=([1]="688 624" [2]="608 448" [3]="848 848" [4]="256 880" [5]="1264 704" [6]="160 608" [7]="208 384" [8]="48 352" [9]="928 512" [10]="1216 512" [11]="720 176" [12]="720 224")
: > ckr800.done
for L in ${LEVELS:-1 2 3 4 6 11}; do
 CK=$(python3 -c "import json;print(' '.join(f'{x} {y}' for x,y in json.load(open('ckpts.json'))['$L']))")
 N=$(python3 -c "import json;print(len(json.load(open('ckpts.json'))['$L']))")
 for v in ${VS:-vb vb91}; do
  rm -f cr_${v}_$L.*
  BENCH_S1990=0 BENCH_GOD=2 BENCH_CKPT="$CK" BENCH_SEG=3 BENCH_JUMPT=0.5 BENCH_LEG=1.5 BENCH_SECS=$((N*3+2)) timeout 290 ./runp.sh ../$v cr_${v}_$L $L "${W[$L]}" CBIOS_MSX2+_R800 >/dev/null 2>&1
  python3 - <<PY >> ckr800.done
try:
    rows=[l.split() for l in open('cr_${v}_$L.txt.ck')]
    r=sum(int(x[3]) for x in rows);t=sum(float(x[5]) for x in rows)
    sr=sum(int(x[9]) for x in rows);st=sum(float(x[11]) for x in rows)
    print('L$L $v total=%.2f steady=%.2f'%(r/t,sr/st if st else 0))
except Exception as e:
    print('L$L $v ERR',e)
PY
 done
done
echo DONE >> ckr800.done
