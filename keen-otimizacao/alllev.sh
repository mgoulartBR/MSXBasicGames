#!/bin/bash
B=/tmp/claude-0/-home-user-MSXBasicGames/cebb32c5-1abc-529f-a253-d0e39cbf9a57/scratchpad/bench
cd $B
declare -A W=([1]="688 624" [2]="608 448" [3]="848 848" [4]="256 880" [5]="1264 704" [6]="160 608" [7]="208 384" [8]="48 352" [9]="928 512" [10]="1216 512")
for L in ${LEVELS:-1 2 3 4 5 6 7 8 9 10}; do
  for R in base timp; do
    P=$B/../work/development062; [ $R = timp ] && P=$B/../timp
    ./shots.sh $P ${R}_L$L $L "${W[$L]}" "{-1 -1 0 4}" >/dev/null 2>&1
  done
  echo "== level $L"
  python3 -I - <<PY
import numpy as np
from PIL import Image
try:
    a=np.array(Image.open('shots/base_L$L/s0.png').convert('RGB')).astype(int);b=np.array(Image.open('shots/timp_L$L/s0.png').convert('RGB')).astype(int)
    d=(np.abs(a-b).sum(2)>0);print('screen diff pixels',int(d.sum()))
    if d.sum():
        ys,xs=np.nonzero(d);print(' bbox',xs.min(),xs.max(),ys.min(),ys.max())
        Image.fromarray(np.concatenate([a,b,np.repeat((d*255)[:,:,None],3,2)],1).astype(np.uint8)).save('dl_$L.png')
except Exception as e:print('ERR',e)
PY
  python3 -I verify_timp.py ../timp $L shots/base_L$L/s0.vram shots/timp_L$L/s0.vram 2>&1 | tail -4
done
