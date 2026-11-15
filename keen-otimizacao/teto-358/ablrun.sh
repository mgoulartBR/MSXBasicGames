#!/bin/bash
# uso: ablrun.sh <rotulo> "<-D flags>" ; builda variante e mede Hill a 3,58 MHz
L=$1; F="$2"
B=/tmp/claude-0/-home-user-MSXBasicGames/cebb32c5-1abc-529f-a253-d0e39cbf9a57/scratchpad
cd $B/vb/src
cat > ablate_cfg.h <<EOF
$(for d in $F; do echo "#define ${d//=/ }"; done)
EOF
python3 - <<PY
s=open('ablate.h').read()
if 'ablate_cfg.h' not in s: open('ablate.h','w').write('#include "ablate_cfg.h"\n'+s)
PY
cd $B/vb && MSXGL_ROOT=$PWD/../work/deps/MSXgl bash scripts/build.sh >/dev/null 2>&1
for t in make_game_test make_world_test make_keydoor_test; do python3 tests/$t.py >/dev/null 2>&1; done
cd $B/bench && BENCH_Z80=3579545 BENCH_PLACE="1680 440" BENCH_SECS=30 BENCH_SHOT=1 BENCH_SHOTT="12 25" timeout 280 ./runp.sh ../vb $L 1 "688 624" 2>&1 | grep fps | sed "s/^/$L: /"
