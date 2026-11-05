"""Compare exact unknown/visible/remembered terrain pixels from real VDP dumps."""
from pathlib import Path
import re,json,hashlib
import numpy as np
R=Path(__file__).resolve().parents[1];T=R/'tests';checks={}
def check(n,b):
 checks[n]=bool(b);assert b,n
def pix(raw,v):
 b=np.frombuffer(raw,np.uint8)
 if v=='V9990':i=np.arange(524288,dtype=np.uint32);b=b[(i>>1)|((i&1)<<18)];w=512
 else:w=256
 b=b.reshape(-1,w//2);a=np.empty((len(b),w),np.uint8);a[:,::2]=b>>4;a[:,1::2]=b&15;return a
raw=b''.join((R/f'art07/runtime/static{i}.bin').read_bytes() for i in range(3));b=np.frombuffer(raw,np.uint8).reshape(-1,8);atlas=np.empty((len(b),16),np.uint8);atlas[:,::2]=b>>4;atlas[:,1::2]=b&15;atlas=atlas.reshape(-1,16,16)
mapping=list(map(int,re.search(r'tile_source\[\]=\{(.*?)\}',(R/'src/art07.h').read_text())[1].split(',')))
for v in ['V9990','PREVIEW']:
 for state,expected_state in [('unknown',0),('visible',2),('remembered',1)]:
  pre=T/f'fog011-{v}-{state}';page,cx,cy=map(int,Path(str(pre)+'-meta.txt').read_text().split());a=pix(Path(str(pre)+'-vram.bin').read_bytes(),v)
  amap=Path(str(pre)+'-memory.bin').read_bytes();seen=Path(str(pre)+'-seen.bin').read_bytes();visible=Path(str(pre)+'-visible.bin').read_bytes()
  # Empty terrain cell at (3,18), away from the scout sprite and tree crowns.
  cell=18*32+3;mask=1<<(cell&7);s=2 if visible[cell>>3]&mask else 1 if seen[cell>>3]&mask else 0
  check(v+'_'+state+'_logical',s==expected_state)
  tile=atlas[mapping[amap[cell]]].copy()
  if s==0:tile[:]=1
  elif s==1:
   yy,xx=np.indices((16,16));tile[(xx+yy)%2==0]=1
  yy=np.arange(max(288,cy),min(304,cy+160));xx=np.arange(max(48,cx),min(64,cx+256));assert len(yy) and len(xx)
  tile=tile[np.ix_(yy-288,xx-48)]
  actual=a[np.ix_(page*256+(yy-cy if v=='V9990' else yy%256),xx-cx if v=='V9990' else xx%256)]
  check(v+'_'+state+'_exact_pixels',np.array_equal(tile,actual))
  if state=='unknown':
   frame=a[np.ix_(page*256+(np.arange(cy,cy+160)-cy if v=='V9990' else np.arange(cy,cy+160)%256),np.arange(cx,cx+256)-cx if v=='V9990' else np.arange(cx,cx+256)%256)]
   check(v+'_unknown_field_no_enemies_or_structures',np.all(frame==1))
for v in ['V9990','PREVIEW']:
 def world_frame(pre,meta_order=False):
  meta=list(map(int,Path(str(pre)+'-meta.txt').read_text().split()))
  if meta_order:cx,cy,page=meta[:3]
  else:page,cx,cy=meta[:3]
  a=pix(Path(str(pre)+'-vram.bin').read_bytes(),v)
  actual=a[np.ix_(page*256+(np.arange(cy,cy+160)-cy if v=='V9990' else np.arange(cy,cy+160)%256),np.arange(cx,cx+256)-cx if v=='V9990' else np.arange(cx,cx+256)%256)]
  memory=Path(str(pre)+'-memory.bin').read_bytes();seen=Path(str(pre)+'-seen.bin').read_bytes();vis=Path(str(pre)+'-visible.bin').read_bytes()
  expected=np.ones((160,256),np.uint8)
  for yy in range(160):
   wy=cy+yy
   for xx in range(256):
    wx=cx+xx;c=(wy//16)*32+wx//16;m=1<<(c&7)
    if not seen[c>>3]&m:continue
    tile=memory[c];color=int(atlas[mapping[tile],wy%16,wx%16]);color=3 if tile>=64 and color==0 else color
    expected[yy,xx]=1 if not vis[c>>3]&m and (wx+wy)%2==0 else color
  return actual,expected,a
 for state in ['corner','middle']:
  actual,expected,_=world_frame(T/f'camera07-{v}-{state}',True)
  check(v+'_camera_'+state+'_exact_fog_viewport',np.array_equal(actual,expected))
 for state in ['first','moved','cancelled']:
  pre=T/f'overlay011-{v}-{state}';actual,expected,_=world_frame(pre)
  meta=list(map(int,Path(str(pre)+'-meta.txt').read_text().split()))
  if meta[3]:
   x=192 if state=='first' else 128;color=10 if meta[4] else 7
   expected[0,x:x+48]=color;expected[47,x:x+48]=color;expected[:48,x]=color;expected[:48,x+47]=color
  for x in [128,192]:check(v+'_ghost_'+state+'_'+str(x),np.array_equal(actual[:48,x:x+48],expected[:48,x:x+48]))
 # The exact font and military context on both hidden HUD panels survive fog.
 a=pix((T/f'fog011-{v}-remembered-vram.bin').read_bytes(),v)
 raw=np.frombuffer((R/'art07/runtime/font.bin').read_bytes(),np.uint8).reshape(16,128);font=np.empty((16,256),np.uint8);font[:,::2]=raw>>4;font[:,1::2]=raw&15
 check(v+'_font_unchanged',np.array_equal(a[1216:1232,:256] if v=='V9990' else a[640:656,:256],font))
 expected='SWORD HP 048/048 A08 D0'.ljust(29)
 for panel in range(2):
  yy=panel*64+16+(0 if v=='V9990' else 512);xx0=256 if v=='V9990' else 0
  text=''
  for j in range(29):
   block=a[yy:yy+8,xx0+8+j*8:xx0+16+j*8]
   matches=[chr(c+32) for c in range(64) if np.array_equal(block,font[(c>>5)*8:(c>>5)*8+8,(c&31)*8:(c&31)*8+8])]
   text+=matches[0] if matches else '?'
  check(v+'_hud_context_panel'+str(panel),text==expected)
report={'passed':True,'checks':len(checks),'assertions':checks,'rom_sha256':{n:hashlib.sha256((R/(n+'.ROM')).read_bytes()).hexdigest() for n in ['KAW9990','KAW68PV','KAW9968']}}
(T/'fog011.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(checks),'Fog of War pixel checks')
