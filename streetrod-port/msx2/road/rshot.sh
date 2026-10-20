#!/bin/sh
# uso: rshot.sh N  -> tira N capturas de corrida (state=1) em $D e monta rs/all.png
cd "$(dirname "$0")"
D=/tmp/claude-0/-home-user-MSXBasicGames/fcd63da7-a365-57b2-8612-d6eea0217e5d/scratchpad/rs
rm -rf $D; mkdir -p $D
NSNAP=${1:-3} SHOTDIR=$D SDL_AUDIODRIVER=dummy timeout 280 xvfb-run -a openmsx -machine C-BIOS_MSX2 -cart ${ROM:-road.rom} -script race_snap.tcl >/dev/null 2>&1
for f in $D/r*.vram; do n=${f%.vram}; python3 vram2png.py $f $(cat $n.r2) $n.png 2>/dev/null; done
python3 - <<PY
from PIL import Image
import glob
fs=sorted(glob.glob('$D/r*.png'))
ims=[Image.open(f).crop((0,0,768,500)) for f in fs]
o=Image.new('RGB',(768,500*len(ims)))
for i,im in enumerate(ims): o.paste(im,(0,i*500))
o.save('$D/all.png'); print(len(fs))
PY
