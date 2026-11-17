#!/bin/bash
declare -A W=([1]="688 624" [2]="608 448" [3]="848 848" [4]="256 880" [5]="1264 704" [6]="160 608" [7]="208 384" [8]="48 352" [9]="928 512" [10]="1216 512" [11]="720 176" [12]="720 224")
: > ckall.done
for L in 1 2 3 4 5 6 7 8 9 10 11 12; do
 CK=$(python3 -c "import json;print(' '.join(f'{x} {y}' for x,y in json.load(open('ckpts.json'))['$L']))")
 N=$(python3 -c "import json;print(len(json.load(open('ckpts.json'))['$L']))")
 for v in vb975 vb; do
  rm -f ck_${v}_$L.*
  BENCH_GOD=1 BENCH_CKPT="$CK" BENCH_SEG=3 BENCH_JUMPT=0.5 BENCH_LEG=1.5 BENCH_Z80=5369318 BENCH_SECS=$((N*3+2)) timeout 290 ./runp.sh ../$v ck_${v}_$L $L "${W[$L]}" >/dev/null 2>&1
  python3 - <<PY >> ckall.done
import re
try:
    rows=[l.split() for l in open('ck_${v}_$L.txt.ck')]
    r=sum(int(x[3]) for x in rows);t=sum(float(x[5]) for x in rows)
    print('L$L $v segs=%d renders=%d secs=%.1f fps=%.2f'%(len(rows),r,t,r/t if t else 0),' '.join(x[7] for x in rows))
except Exception as e:
    print('L$L $v ERR',e)
PY
 done
done
echo DONE >> ckall.done
