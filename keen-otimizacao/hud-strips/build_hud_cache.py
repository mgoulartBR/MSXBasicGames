"""Reserve unused atlas rectangles for the HUD without relocating any source."""
from pathlib import Path
import re,json,numpy as np
r=Path(__file__).resolve().parents[1];s=(r/'src/combat_assets.c').read_text();allslots=[]
for j,name in enumerate(['scene','melon','bridge','rapids','pass','city','apple','grove','bay','swamp','boobus','castle']):
 used=np.zeros((512,256),dtype=bool);used[:128 if name=="boobus" else 96]=True
 def mark(x,y,w,h):
  if w and h:used[max(0,y):min(512,y+h),max(0,x):min(256,x+w)]=True
 if j<3:
  block=re.search(r'static const SpriteDef sd'+str(j)+r'\[\d*\]=\{(.*?)\};',s,re.S)[1]
  for i,v in enumerate(re.findall(r'\{([0-9,]+)\}',block)):
   if i==35:continue
   mx,cx,my,cy,w,h,ox,oy=map(int,v.split(','))
   mark(mx,my+(256 if ox&128 else 0),w,h);mark(cx,cy+(256 if oy&128 else 0),w,h)
  tx=list(map(int,re.search(r'tx'+str(j)+r'\[\]=\{(.*?)\}',s)[1].split(',')));ty=list(map(int,re.search(r'ty'+str(j)+r'\[\]=\{(.*?)\}',s)[1].split(',')))
  for x,y in zip(tx,ty):mark(x,y-512,16,16)
 else:
  layout=json.loads((r/'assets'/name/'combat_assets.json').read_text())
  for i,sp in enumerate(layout['sprites']):
   if i==35 or any(k.startswith('streamed_') and v for k,v in sp.items()):continue
   left,top,w,h=sp['crop']
   for key in ['mask','color']:
    x,y=sp[key];mark(x+left,y+top,w,h)
  for x,y in layout['tiles']:mark(x,y,16,16)
 def find(w,h):
  for y in range(512-h,95,-1):
   for x in range(0,256-w+1,2):
    if not used[y:y+h,x:x+w].any():return (x,y)
  return None
 # Prefer whole-width strips: 72x16 (2 copies per redraw), then 72x8 (4), else 8x8 cells.
 strips=None
 import os
 for sh in ((16,8) if os.environ.get('HUD_STRIPS') else ()):
  snap=used.copy();got=[]
  for k in range(32//sh):
   f=find(72,sh)
   if not f:got=None;break
   mark(f[0],f[1],72,sh);got.append([f[0],f[1]+512,0,k*sh,72,sh])
  if got:strips=got;break
  used[:]=snap
 if strips:
  allslots.append(('strips',strips));print(name,'strips',strips);continue
 slots=[]
 for k in range(36):
  found=None
  for y in range(504,95,-1):
   if y+8>512:continue
   for x in range(0,249,2):
    if not used[y:y+8,x:x+8].any():found=(x,y);break
   if found:break
  assert found,(name,k,'no 8x8 free slot')
  x,y=found;assert not used[y:y+8,x:x+8].any();mark(x,y,8,8);slots.append([x,y+512])
 allslots.append(slots);print(name,slots)
# Merge neighbouring cells only when both source and destination are contiguous.
merged=[]
for slots in allslots:
 if isinstance(slots,tuple):merged.append(slots[1]);continue
 rects=[]
 for k,(x,y) in enumerate(slots):
  dx=k%9*8;dy=k//9*8
  if rects and rects[-1][1]==y and rects[-1][3]==dy and rects[-1][0]+rects[-1][4]==x and rects[-1][2]+rects[-1][4]==dx:
   rects[-1][4]+=8
  else:rects.append([x,y,dx,dy,8,8])
 merged.append(rects)
maxn=max(map(len,merged))
(r/'src/hud_cache.h').write_text('// Generated from source occupancy. No atlas source is relocated.\nstatic const u8 hud_chunks_count[12]={'+','.join(str(len(a)) for a in merged)+'};\ntypedef struct {u8 sx;u16 sy;u8 dx,dy,w,h;} HudChunk;\nstatic const HudChunk hud_chunks[12]['+str(maxn)+']={'+','.join('{'+','.join('{'+','.join(map(str,a))+'}' for a in rects)+'}' for rects in merged)+'};\n')
(r/'build').mkdir(exist_ok=True);(r/'build/hud-cache.json').write_text(json.dumps(merged,indent=2))
print('Merged copy commands per stage:',list(map(len,merged)))
