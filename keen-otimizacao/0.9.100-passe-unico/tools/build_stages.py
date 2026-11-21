#!/usr/bin/env python3
"""Pack cropped EGA actors and composed tiles; run after build_scene.py."""
from pathlib import Path
import struct,json,hashlib,sys
PACK_STAGES={1} if "--melon-only" in sys.argv else {0} if "--hill-only" in sys.argv else {0,1,2}
import numpy as np
from import_ega import planar
R=Path(__file__).resolve().parents[1];P=R/'assets/scene';G=R/'assets/generated'
maps=json.loads((G/'maps.json').read_text());stages=[next(m for m in maps if m['id']==i) for i in (1,2,3)]
keys_by_stage=[sorted(set(zip(*m['planes'][:2]))) for m in stages]
count=max(map(len,keys_by_stage));assert count==352
mh=(G/'MAPHEAD.KDR').read_bytes() if (G/'MAPHEAD.KDR').exists() else (R.parent/'upstream/keen/static/MAPHEAD.KDR').read_bytes()
items=[];defs=[];tilexy=[None]*count
stage_pixels=[]
for keys in keys_by_stage:
 pixels=[]
 for bg,fg in keys:
  path=G/f'chunk_{370+bg:04}.bin'
  pix=planar(path.read_bytes())[0] if path.exists() else [0]*256
  if fg:
   front,mask=planar((G/f'chunk_{1810+fg:04}.bin').read_bytes(),masked=True)
   if not path.exists():assert not any(mask)
   pix=[(b if a else 0)|f for b,f,a in zip(pix,front,mask)]
  pixels.append(pix)
 stage_pixels.append(pixels+[[0]*256 for _ in range(count-len(pixels))])
for i in range(count):items.append(('tile',i,16,16,stage_pixels[0][i]+stage_pixels[1][i]))
ids=list(range(72,88))+list(range(261,271))+list(range(164,168))+[108,117,162,163,190,None]+list(range(138,148))
meta=(G/'chunk_0002.bin').read_bytes()
for f,ch in enumerate(ids):
 if ch is None:
  defs.append({'chunk':None,'width':48,'height':16,'crop':[0,0,48,16],'mask':[0,0],'color':[0,0]});continue
 m=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=m[0]*8,m[1];ox,oy=m[2]//16,m[3]//16
 w,h=(32,32) if 16<=f<26 else (8,8) if 32<=f<34 else (16,24) if f==34 else (24,32)
 if f>=36:w=max(w,ox+fw);h=max(h,oy+fh)
 if f==34:ox=oy=0
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True)
 cp=[0]*(w*h);mp=[15]*(w*h)
 for y in range(fh):
  for x in range(fw):
   if 0<=x+ox<w and 0<=y+oy<h:
    j=(y+oy)*w+x+ox;k=y*fw+x;cp[j]=col[k];mp[j]=15 if mask[k] else 0
 pts=[(i%w,i//w)for i,v in enumerate(mp)if v==0]
 l=min(x for x,y in pts)//2*2;r=(max(x for x,y in pts)+2)//2*2;t=min(y for x,y in pts);b=max(y for x,y in pts)+1
 defs.append({'chunk':ch,'width':w,'height':h,'crop':[l,t,r-l,b-t]})
 for part,pix in [('mask',mp),('color',cp)]:items.append((part,f,r-l,b-t,[pix[y*w+x]for y in range(t,b)for x in range(l,r)]))
# Shared sprite positions, with the union of original foreground overlays.
base=502+2880+5*1206
foreground={};fg_lookups=[]
for keys in keys_by_stage:
 fg_lookup=[]
 for bg,fg in keys:
  if fg and mh[base+fg]&128:
   if fg not in foreground:
    f=len(defs)
    if f==84:
     defs.extend([{'chunk':None,'width':1,'height':1,'crop':[0,0,1,1],'mask':[0,0],'color':[0,0]} for _ in range(18)])
     f=len(defs)
    foreground[fg]=f
    col,mask=planar((G/f'chunk_{1810+fg:04}.bin').read_bytes(),16,16,True)
    pts=[(i%16,i//16) for i,v in enumerate(mask) if not v]
    l=min(x for x,y in pts)//2*2;r=(max(x for x,y in pts)+2)//2*2;t=min(y for x,y in pts);b=max(y for x,y in pts)+1
    defs.append({'chunk':1810+fg,'width':16,'height':16,'crop':[l,t,r-l,b-t]})
    items.extend([('mask',f,r-l,b-t,[15 if mask[y*16+x] else 0 for y in range(t,b) for x in range(l,r)]),('color',f,r-l,b-t,[col[y*16+x] for y in range(t,b) for x in range(l,r)])])
   fg_lookup.append(foreground[fg])
  else:fg_lookup.append(0)
 fg_lookups.append(fg_lookup)
# Tomatooth sprites stay resident in VRAM; no per-frame ROM uploads.
tomato_defs=[];tomato_items=[]
for f,ch in enumerate(range(257,261),90):
 m=struct.unpack_from('<9h',meta,(ch-71)*18);w,h=m[0]*8,m[1]
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),w,h,True)
 pts=[(i%w,i//w) for i,v in enumerate(mask) if not v]
 l=min(x for x,y in pts)//2*2;r=(max(x for x,y in pts)+2)//2*2;t=min(y for x,y in pts);b=max(y for x,y in pts)+1
 tomato_defs.append({'chunk':ch,'width':w,'height':h,'crop':[l,t,r-l,b-t]})
 for part,pix in [('mask',[15 if v else 0 for v in mask]),('color',col)]:
  tomato_items.append((part,f,r-l,b-t,[pix[y*w+x] for y in range(t,b) for x in range(l,r)]))
# Bridge-only original enemies. A 24px left pad accommodates negative EGA
# attack offsets without changing the unsigned SpriteDef format.
bridge_defs=[];bridge_items=[];bridge_boxes=[]
bridge_chunks=list(range(241,257))+list(range(281,293))+list(range(309,313))+list(range(315,319))
for f,ch in enumerate(bridge_chunks,108):
 m=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=m[0]*8,m[1];ox,oy=m[2]//16+24,m[3]//16+4
 w,h=ox+fw,oy+fh
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True)
 pts=[(i%fw+ox,i//fw+oy) for i,v in enumerate(mask) if not v]
 l=min(x for x,y in pts)//2*2;r=(max(x for x,y in pts)+2)//2*2;t=min(y for x,y in pts);b=max(y for x,y in pts)+1
 bridge_defs.append({'chunk':ch,'width':w,'height':h,'crop':[l,t,r-l,b-t]})
 bridge_boxes.append([v//16 for v in m[4:8]])
 for part in ('mask','color'):
  pix=[]
  for y in range(t,b):
   for x in range(l,r):
    k=(y-oy)*fw+x-ox;inside=ox<=x<ox+fw and oy<=y<oy+fh
    pix.append((15 if not inside or mask[k] else 0) if part=='mask' else (col[k] if inside else 0))
  bridge_items.append((part,f,r-l,b-t,pix))
(R/'src/bridge_boxes.h').write_text('// Original EGA inclusive hitboxes relative to actor origin.\nstatic const i8 bridge_boxes[36][4]={'+','.join('{'+','.join(map(str,v))+'}' for v in bridge_boxes)+'};\n')
# Melon-only frames reuse unused Brocco/left-Melon slots; table stays 144.
melon_defs={};melon_items=[];mines_boxes=[]
for f,ch in list(zip(range(108,116),range(271,279)))+[(136,313),(137,314),(142,319),(143,320)]:
 m=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=m[0]*8,m[1];ox,oy=m[2]//16+24,m[3]//16+4
 w,h=ox+fw,oy+fh;col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True)
 pts=[(i%fw+ox,i//fw+oy) for i,v in enumerate(mask) if not v]
 l=min(x for x,y in pts)//2*2;rr=(max(x for x,y in pts)+2)//2*2;t=min(y for x,y in pts);bb=max(y for x,y in pts)+1
 melon_defs[f]={'chunk':ch,'width':w,'height':h,'crop':[l,t,rr-l,bb-t]}
 if f<116:mines_boxes.append([v//16 for v in m[4:8]])
 for part in ('mask','color'):
  pix=[]
  for y in range(t,bb):
   for x in range(l,rr):
    k=(y-oy)*fw+x-ox;inside=ox<=x<ox+fw and oy<=y<oy+fh
    pix.append((15 if not inside or mask[k] else 0) if part=='mask' else (col[k] if inside else 0))
  melon_items.append((part,f,rr-l,bb-t,pix))
# All four walking drawings per direction, streamed/cached outside the viewport.
aspar_data=bytearray()
for ch in range(271,279):
 m=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=m[0]*8,m[1];ox,oy=m[2]//16,m[3]//16
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True)
 cp=[0]*640;mp=[15]*640
 for y in range(fh):
  for x in range(fw):
   j=(y+oy)*40+x+ox;k=y*fw+x;cp[j]=col[k];mp[j]=15 if mask[k] else 0
 for y in range(16):
  for pix in (mp,cp):aspar_data.extend((pix[y*40+x]<<4)|pix[y*40+x+1] for x in range(0,40,2))

def timp_rows(data,w,h,n):
 """Old mask|colour rows (w bytes per row) -> one-pass TIMP rows (w/2 bytes): 0 transparent, colour, 9 = second black."""
 out=[]
 for i in range(n):
  d=data[i*w*h:(i+1)*w*h];row=bytearray()
  for y in range(h):
   m=d[y*w:y*w+w//2];c=d[y*w+w//2:(y+1)*w]
   def px(a,x):return a[x//2]&15 if x&1 else a[x//2]>>4
   vals=[]
   for x in range(w):
    mm,cc=px(m,x),px(c,x);assert mm in (0,15) and not(mm==15 and cc) and not(mm==0 and cc==9)
    vals.append(0 if mm==15 else (cc or 9))
   row.extend((vals[x]<<4)|vals[x+1] for x in range(0,w,2))
  out.append(row)
 return out
def timp_array(name,rows):
 return 'static const u8 %s[%d][%d]={'%(name,len(rows),len(rows[0]))+','.join('{'+','.join(map(str,r))+'}' for r in rows)+'};\n'
(R/'assets/melon/aspar.bin').write_bytes(aspar_data)
(R/'src/aspar_images.h').write_text('// Original EGA 271..278, one-pass TIMP rows (0 transparent, 9 second black).\n'+timp_array('aspar_images',timp_rows(aspar_data,40,16,8)))
mouth_data=bytearray()
for ch in (313,314,311,312):
 m=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=m[0]*8,m[1];ox,oy=m[2]//16,m[3]//16
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True);cp=[0]*1024;mp=[15]*1024
 for y in range(fh):
  for x in range(fw):
   j=(y+oy)*32+x+ox;k=y*fw+x;cp[j]=col[k];mp[j]=15 if mask[k] else 0
 for y in range(32):
  for pix in (mp,cp):mouth_data.extend((pix[y*32+x]<<4)|pix[y*32+x+1] for x in range(0,32,2))
(R/'assets/melon/mouth.bin').write_bytes(mouth_data)
with (R/'src/aspar_images.h').open('a') as f:f.write('\n'+timp_array('mouth_images',timp_rows(mouth_data,32,32,4)))

tater_data=bytearray()
for ch in (285,286,291,292):
 m=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=m[0]*8,m[1];ox,oy=m[2]//16+16,m[3]//16
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True);cp=[0]*2048;mp=[15]*2048
 for y in range(fh):
  for x in range(fw):
   j=(y+oy)*64+x+ox;k=y*fw+x;cp[j]=col[k];mp[j]=15 if mask[k] else 0
 for y in range(32):
  for pix in (mp,cp):tater_data.extend((pix[y*64+x]<<4)|pix[y*64+x+1] for x in range(0,64,2))
(R/'assets/melon/tater_attack.bin').write_bytes(tater_data)
(R/'src/tater_images.h').write_text('// Original Tater attack drawings, one-pass TIMP rows (0 transparent, 9 second black).\n'+timp_array('tater_images',timp_rows(tater_data,64,32,4)))
(R/'src/mines_boxes.h').write_text('// Original Asparagusto hitboxes.\nstatic const i8 mines_boxes[8][4]={'+','.join('{'+','.join(map(str,v))+'}' for v in mines_boxes)+'};\n')
# Static original bonus frames; collection state lives in a compact bitset.
bonus_defs=[];bonus_items=[]
for f,ch in enumerate(range(172,188,2),94):
 m=struct.unpack_from('<9h',meta,(ch-71)*18);w,h=m[0]*8,m[1]
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),w,h,True)
 bonus_defs.append({'chunk':ch,'width':w,'height':h,'crop':[0,0,w,h]})
 for part,pix in [('mask',[15 if v else 0 for v in mask]),('color',col)]:bonus_items.append((part,f,w,h,pix))
# Pack shared actors once, then pack each level's deduplicated tiles independently.
def place(items,free,atlas,stage,recipe=(0,0,25)):
 order,strategy,overlap=recipe
 aliases={};positions={}
 occupied=np.ones((512,128),dtype=bool)
 for fx,fy,fw,fh in free:occupied[fy:fy+fh,fx//2:(fx+fw)//2]=False
 reserved=occupied.copy()

 forced={}
 if stage==2 or (stage==1 and order==3):
  unique={}
  for part,i,w,h,pix in items:
   if h>16:unique.setdefault((w,h,bytes(pix)),(part,i,w,h,pix))
  remaining=list(unique.values());yy=96
  while remaining:
   dp={0:(0,[])}
   for j,(_,_,w,h,pix) in enumerate(remaining):
    for width,(score,chosen) in list(dp.items())[::-1]:
     new=width+w;value=score+w*(1<<(h*2))
     if new<=256 and (new not in dp or value>dp[new][0]):dp[new]=(value,chosen+[j])
   chosen=dp[max(dp)][1];xx=0;rh=max(remaining[j][3] for j in chosen)
   row_data=np.zeros((rh,128),dtype=np.uint8);row_used=np.zeros((rh,128),dtype=bool)
   for j in sorted(chosen,key=lambda j:remaining[j][0]):
    part,i,w,h,pix=remaining[j];raw=np.array(pix,dtype=np.uint8).reshape(h,w);raw=(raw[:,::2]<<4)|raw[:,1::2]
    for xx2 in range(0,xx+1,2):
     prior=row_data[:h,xx2//2:(xx2+w)//2];used=row_used[:h,xx2//2:(xx2+w)//2]
     if prior.shape==raw.shape and np.all(~used | (prior==raw)):break
    forced[(w,h,bytes(pix))]=(xx2,yy)
    row_data[:h,xx2//2:(xx2+w)//2]=raw;row_used[:h,xx2//2:(xx2+w)//2]=True;xx=max(xx,xx2+w)
   print('shelf',yy,rh,xx,flush=True)
   yy+=rh;remaining=[v for j,v in enumerate(remaining) if j not in chosen]
  print('Tall shelves end',yy,flush=True)
  # Preserve a straight edge for the 16x16 tile rows. Tiny gaps between
  # cropped actor bottoms must not stagger those rows.
  if stage==2:free=[(fx,max(fy,yy),fw,fy+fh-max(fy,yy)) for fx,fy,fw,fh in free if fy+fh>yy]


 if stage==1 and order==2:
  tiles=list(dict.fromkeys((w,h,bytes(pix)) for part,i,w,h,pix in items if part=='tile'))
  for n,key in enumerate(tiles):forced[key]=((n%16)*16,96+(n//16)*16)
 def sortkey(q):
  if stage!=1 or order in (0,3):return (0 if q[3]>16 else 1,-max(q[2:4]),-q[2]*q[3])
  if order in (1,2):return (0 if q[0]=='tile' else 1,-max(q[2:4]),-q[2]*q[3])
  if order==4:return (-q[2]*q[3],-q[3],-q[2])
  return (-q[3],-q[2],q[0])
 for part,i,w,h,pix in sorted(items,key=sortkey):
  key=(w,h,bytes(pix))
  if key in aliases:positions[(part,i)]=aliases[key];continue
  fits=[(([min(fw-w,fh-h),fw*fh-w*h,max(fw-w,fh-h),fy,fw-w,fh-h][strategy%6]),max(fw-w,fh-h),fy,fx,j)for j,(fx,fy,fw,fh)in enumerate(free)if w<=fw and h<=fh]
  if key in forced:
   x,y=forced[key]
  else:
   # Exact byte-compatible overlap: transparent margins and repeated solid
   # spans can share VRAM. Reserved hardware tables never participate.
   pw=w//2;ph=h;ab=np.frombuffer(atlas,dtype=np.uint8).reshape(512,128)
   raw=np.array(pix,dtype=np.uint8).reshape(h,w);packed=(raw[:,::2]<<4)|raw[:,1::2]
   okay=np.ones((513-ph,129-pw),dtype=bool)
   shared=np.zeros(okay.shape,dtype=np.uint16)
   for yy in range(ph):
    for xx in range(pw):
     used=occupied[yy:yy+513-ph,xx:xx+129-pw]
     shared+=used
     okay &= ~reserved[yy:yy+513-ph,xx:xx+129-pw] & (~used | (ab[yy:yy+513-ph,xx:xx+129-pw]==packed[yy,xx]))
   candidates=np.argwhere(okay)
   shared[~okay]=0
   if len(candidates) and (not fits or shared.max()>=(max(1,pw*ph*overlap//100) if stage==1 else pw*ph//4)):
    y,x=map(int,np.unravel_index(shared.argmax(),shared.shape)) if shared.max() else map(int,candidates[0]);x*=2
   else:
    assert fits,('Atlas overflow',part,i,w,h,'free bytes',65536-int(occupied.sum()),'placed',len(positions))
    _,_,y,x,_=min(fits)
  occupied[y:y+h,x//2:(x+w)//2]=True
  nextfree=[]
  for fx,fy,fw,fh in free:
   if x>=fx+fw or x+w<=fx or y>=fy+fh or y+h<=fy:nextfree.append((fx,fy,fw,fh));continue
   if x>fx:nextfree.append((fx,fy,x-fx,fh))
   if x+w<fx+fw:nextfree.append((x+w,fy,fx+fw-x-w,fh))
   if y>fy:nextfree.append((fx,fy,fw,y-fy))
   if y+h<fy+fh:nextfree.append((fx,y+h,fw,fy+fh-y-h))
  free=[a for j,a in enumerate(nextfree)if not any(j!=k and b[0]<=a[0] and b[1]<=a[1] and b[0]+b[2]>=a[0]+a[2] and b[1]+b[3]>=a[1]+a[3] and (a!=b or k<j)for k,b in enumerate(nextfree))]
  for row in range(h):atlas[(y+row)*128+x//2:(y+row)*128+x//2+w//2]=bytes((pix[row*w+k]<<4)|pix[row*w+k+1]for k in range(0,w,2))
  positions[(part,i)]=(x,y);aliases[key]=(x,y)
 return positions,free
atlases=[];tile_tables=[];sprite_tables=[]
import copy
for stage,pixels in enumerate(stage_pixels):
 if stage not in PACK_STAGES:
  out=R/'assets'/('scene','melon','bridge')[stage]
  layout=json.loads((out/'combat_assets.json').read_text())
  atlases.append((out/'atlas.bin').read_bytes());sprite_tables.append(layout['sprites']);tile_tables.append(layout['tiles']);continue
 atlas=bytearray(65536);used=set(fg_lookups[stage])
 stage_items=[('tile',i,16,16,pix) for i,pix in enumerate(pixels)]
 stage_items += [item for item in items if item[0]!='tile' and (item[1]<46 or item[1] in used) and not((item[1]<16 and item[1] not in (0,8)) or item[1] in (30,31) or (stage>=1 and 36<=item[1]<46 and item[1] not in (36,43))) and not(stage>=1 and item[1] in (17,19,22,24,28,29))]
 if stage==0:
  stage_items=[v for v in stage_items if v[0]=='tile' or not(v[1]<16 or 36<=v[1]<46)]
  stage_items+=[v for v in bridge_items if 124<=v[1]<136 and v[1] not in (125,127,131,133)]
 if stage==1:
  stage_items=[v for v in stage_items if v[0]=='tile' or not(v[1]<16 or 36<=v[1]<46)]
  stage_items+=tomato_items+[v for v in melon_items if v[1]>=142]
  stage_items+=[v for v in bridge_items if (124<=v[1]<136 and v[1] not in (125,127,128,129,131,133,134,135)) or 140<=v[1]<142]
 if stage==2:stage_items+=[v for v in bridge_items if v[1] not in (109,111,117,119,125,127,131,133)]
 stage_items+=[v for v in bonus_items if stage!=0 or v[1] not in (99,101)]
 free=[(0,0,256,512)]
 # Two sprite pattern/color/attribute tables; bitmap hero remains for occlusion fallback.
 reservations=[(0,0,256,64),(0,496,48,16)]
 for base_y in (64,80):
  reservations.extend([(0,base_y,256,6),(0,base_y+8,256,3),(0,base_y+12,200,1)])
 for x,y,w,h in reservations:
  clipped=[]
  for fx,fy,fw,fh in free:
   if x>=fx+fw or x+w<=fx or y>=fy+fh or y+h<=fy:clipped.append((fx,fy,fw,fh));continue
   if x>fx:clipped.append((fx,fy,x-fx,fh))
   if x+w<fx+fw:clipped.append((x+w,fy,fx+fw-x-w,fh))
   if y>fy:clipped.append((fx,fy,fw,y-fy))
   if y+h<fy+fh:clipped.append((fx,y+h,fw,fy+fh-y-h))
  free=[a for j,a in enumerate(clipped)if not any(j!=k and b[0]<=a[0] and b[1]<=a[1] and b[0]+b[2]>=a[0]+a[2] and b[1]+b[3]>=a[1]+a[3] and (a!=b or k<j)for k,b in enumerate(clipped))]
 print('AREA',stage,sum(k[0]*k[1]//2 for k in {(w,h,bytes(pix)) for _,_,w,h,pix in stage_items}),flush=True)
 if stage==1:positions,_=place(stage_items,free,atlas,stage,(0,0,50))
 else:positions,_=place(stage_items,free,atlas,stage)
 atlas[:8192]=(R/'assets/scene/hero_resident.bin').read_bytes()
 # Verify every original image after all placements, including shared bytes.
 for part,i,w,h,pix in stage_items:
  x,y=positions[(part,i)]
  for row in range(h):
   expected=bytes((pix[row*w+k]<<4)|pix[row*w+k+1] for k in range(0,w,2))
   assert atlas[(y+row)*128+x//2:(y+row)*128+(x+w)//2]==expected,(stage,part,i,row)

 table=copy.deepcopy(defs)
 for i,d in enumerate(table):
  l,t,_,_=d['crop']
  for part in ('mask','color'):
   x,y=positions.get((part,i),(l,t));d[part]=[x-l,y-t]
 if stage>=1:
  for target,source in [(17,16),(19,18),(22,21),(24,23),(28,26),(29,27)]:table[target]=copy.deepcopy(table[source])
 for i,ch in enumerate((293,294),84):table[i]=({'chunk':ch,'width':48,'height':24,'crop':[0,0,48,24],'mask':[0,0],'color':[0,0],'streamed_cart':True})
 for i,(ch,w,h) in enumerate([(197,16,8),(198,16,8),(211,16,64),(210,8,16)],86):table[i]=({'chunk':ch,'width':w,'height':h,'crop':[0,0,w,h],'mask':[0,0],'color':[0,0],'streamed_object':True})
 for i,d0 in enumerate(tomato_defs,90):
  d=copy.deepcopy(d0);l,t,_,_=d["crop"]
  for part in ("mask","color"):
   x,y=positions.get((part,i),(l,t));d[part]=[x-l,y-t]
  table[i]=d
 for i,d0 in enumerate(bonus_defs,94):
  d=copy.deepcopy(d0)
  for part in ('mask','color'):d[part]=list(positions.get((part,i),(0,0)))
  table[i]=d
 for i,d0 in enumerate(bridge_defs,108):
  d=copy.deepcopy(d0);l,t,_,_=d["crop"]
  for part in ("mask","color"):
   x,y=positions.get((part,i),(l,t));d[part]=[x-l,y-t]
  table.append(d)
 if stage==1:
  for i,d0 in melon_defs.items():
   if i<116:
    table[i]={'chunk':d0['chunk'],'width':64,'height':20,'crop':[24,4,40,16],'mask':[0,0],'color':[0,0],'streamed_aspar':True};continue
   d=copy.deepcopy(d0);l,t,_,_=d['crop']
   if i in (136,137):
    table[i]={'chunk':d0['chunk'],'width':56,'height':36,'crop':[24,4,32,32],'mask':[0,0],'color':[0,0],'streamed_mouth':True};continue
   for part in ('mask','color'):
    x,y=positions[(part,i)];d[part]=[x-l,y-t]
   table[i]=d
  for i,ch in ((128,285),(129,286),(134,291),(135,292)):table[i]={'chunk':ch,'width':72,'height':36,'crop':[8,4,64,32],'mask':[0,0],'color':[0,0],'streamed_tater':True}
  for i,ch in ((138,311),(139,312)):table[i]={'chunk':ch,'width':56,'height':36,'crop':[24,4,32,32],'mask':[0,0],'color':[0,0],'streamed_mouth':True}
  for target,source in [(125,124),(127,126),(131,130),(133,132)]:table[target]=copy.deepcopy(table[source])
 if stage==0:
  for target,source in [(125,124),(127,126),(131,130),(133,132)]:table[target]=copy.deepcopy(table[source])
 if stage==2:
  for target,source in [(109,108),(111,110),(117,116),(119,118),(125,124),(127,126),(131,130),(133,132)]:table[target]=copy.deepcopy(table[source])
 sprite_tables.append(table)
 tile_tables.append([positions[('tile',i)] for i in range(count)]);atlases.append(atlas)
# 9-bit map IDs, plus compact collision and stage-specific metadata in RAM.
def pack9(values):
 out=bytearray((len(values)*9+7)//8)
 for i,v in enumerate(values):
  bit=i*9;out[bit//8]|=(v<<(bit&7))&255
  if bit//8+1<len(out):out[bit//8+1]|=v>>(8-(bit&7))
 return bytes(out)
for stage,(m,keys,atlas,fg_lookup,tilexy,defs) in enumerate(zip(stages,keys_by_stage,atlases,fg_lookups,tile_tables,sprite_tables)):
 out=R/'assets'/('scene','melon','bridge')[stage];out.mkdir(exist_ok=True)
 indices={k:i for i,k in enumerate(keys)};ids=[indices[k] for k in zip(*m['planes'][:2])]
 props=[tuple(mh[3382+d*1206+fg] for d in range(4)) for bg,fg in keys]
 unique=sorted(set(props));assert len(unique)<=32
 data=bytearray(1168)
 data[:len(keys)]=bytes(unique.index(p) for p in props)
 for i,v in enumerate(unique):data[352+i*4:356+i*4]=bytes(v)
 for i,(_,fg) in enumerate(keys):
  if mh[base+fg]&127==1:data[480+i//8]|=1<<(i&7)
 data[524:524+len(keys)]=bytes(fg_lookup)
 pickups=[{'x':i%m['width']*16,'y':i//m['width']*16-(8 if v==30 else 0),'amount':1 if v==29 else 5,'frame':32 if v==29 else 34} for i,v in enumerate(m['planes'][2]) if v in (29,30)]
 assert len(pickups)<=25
 for i,o in enumerate(pickups):data[876+i*6:882+i*6]=struct.pack('<HHBB',o['x'],o['y'],o['amount'],o['frame'])
 spawn=next((i%m['width']*16,i//m['width']*16-16,v) for i,v in enumerate(m['planes'][2]) if v in (1,2))
 struct.pack_into('<BBHHBB',data,1120,m['width'],m['height'],spawn[0],spawn[1],spawn[2]==2,len(pickups))
 (out/'atlas.bin').write_bytes(atlas);(out/'packed_map.bin').write_bytes(pack9(ids));(out/'stage_data.bin').write_bytes(data)
 (out/'map.bin').write_bytes(struct.pack('<'+'H'*len(ids),*ids))
 (out/'pickups.json').write_text(json.dumps(pickups,indent=2))
 (out/'combat_assets.json').write_text(json.dumps({'sprites':defs,'tiles':tilexy,'foreground':fg_lookup},indent=2))
 manifest={'id':m['id'],'name':m['name'],'width':m['width'],'height':m['height'],'tiles':len(keys),'spawn':spawn,'pickups':len(pickups)}
 (out/'stage_manifest.json').write_text(json.dumps(manifest,indent=2))
 scene={'version':'0.8.2','map_id':m['id'],'name':m['name'],'width_tiles':m['width'],'height_tiles':m['height'],'unique_composited_tiles':len(keys),'spawn_pixels':list(spawn[:2]),'sprite_layout':'combat_assets.json','actor_frames':len(defs),'atlas_bytes':65536,'map_encoding':'9-bit little-endian bitstream','sha256':{n:hashlib.sha256((out/n).read_bytes()).hexdigest() for n in ('atlas.bin','packed_map.bin','stage_data.bin')}}
 (out/'scene_manifest.json').write_text(json.dumps(scene,indent=2))
# Tuberia's existing logical IDs also fit the same packed storage.
world=(R/'assets/tuberia/map.bin').read_bytes();(R/'assets/tuberia/packed_map.bin').write_bytes(pack9(struct.unpack('<5610H',world)))
h='// Generated; shared EGA sprite layout.\n#ifndef KEEN_ASSETS_H\n#define KEEN_ASSETS_H\n#include "msxgl.h"\ntypedef struct {u8 mx,cx;u16 my,cy;u8 w,h,ox,oy;} SpriteDef;\nextern const SpriteDef sprites[3][%d];\nextern const u8 tile_x[3][352];\nextern const u16 tile_y[3][352];\n#endif\n'%len(defs)
(R/'src/combat_assets.h').write_text(h)
c='#include "combat_assets.h"\nconst SpriteDef sprites[3][%d]={\n'%len(defs)
for table in sprite_tables:
 c+='{\n'
 for d in table:
  l,t,w,h=d['crop'];c+='{%d,%d,%d,%d,%d,%d,%d,%d},\n'%(d['mask'][0]+l,d['color'][0]+l,512+d['mask'][1]+t,512+d['color'][1]+t,w,h,l,t)
 c+='},\n'
c+='};\nconst u8 tile_x[3][352]={'+','.join('{'+','.join(str(x) for x,y in t)+'}' for t in tile_tables)+'};\nconst u16 tile_y[3][352]={'+','.join('{'+','.join(str(512+y)for x,y in t)+'}' for t in tile_tables)+'};\n';(R/'src/combat_assets.c').write_text(c)
print('Three atlases packed:',len(defs),'sprites;',count,'tile slots.')

# Runtime dimensions come from the selected stage metadata.
p=R/'src/scene_data.h'
import re
p.write_text(re.sub(r'^#define MAP_(?:WIDTH|HEIGHT) .*\n','',p.read_text(),flags=re.M))

bonus_lists=[[(i%m['width']*16,i//m['width']*16,v-21) for i,v in enumerate(m['planes'][2]) if 21<=v<=28] for m in stages]
assert max(map(len,bonus_lists))<=32
(R/'src/bonus_data.h').write_text('// Generated original bonus objects.\nstatic const u8 bonus_count[3]={'+','.join(str(len(a)) for a in bonus_lists)+'};\nstatic const Bonus bonus_data[3][32]={\n'+',\n'.join('{'+(','.join('{%d,%d,%d}'%v for v in a) if a else '{0,0,0}')+'}' for a in bonus_lists)+'};\n')
