#!/usr/bin/env python3
"""Build stage five from the original map and EGA data, preserving stages 1..4."""
from pathlib import Path
import ast,copy,json,struct
import numpy as np
from import_ega import planar
R=Path(__file__).resolve().parents[1];G=R/'assets/generated';O=R/'assets/pass';O.mkdir(exist_ok=True)
maps=json.loads((G/'maps.json').read_text());m=next(a for a in maps if a['id']==5)
meta=(G/'chunk_0002.bin').read_bytes();mh=(G/'MAPHEAD.KDR').read_bytes()
node=next(n for n in ast.parse((R/'tools/build_stages.py').read_text().replace('if stage==2:free=[(fx,max(fy,yy),fw,fy+fh-max(fy,yy)) for fx,fy,fw,fh in free if fy+fh>yy]','pass')).body if isinstance(n,ast.FunctionDef) and n.name=='place')
exec(compile(ast.Module(body=[node],type_ignores=[]),'packer','exec'))
keys=sorted(set(zip(*m['planes'][:2])));items=[];defs=copy.deepcopy(json.loads((R/'assets/rapids/combat_assets.json').read_text())['sprites']);defs.extend([dict(chunk=None,width=1,height=1,crop=[0,0,1,1],mask=[0,0],color=[0,0]) for _ in range(22)])
for i,(bg,fg) in enumerate(keys):
 path=G/f'chunk_{370+bg:04}.bin';pix=planar(path.read_bytes())[0] if path.exists() else [0]*256
 if fg:
  col,mask=planar((G/f'chunk_{1810+fg:04}.bin').read_bytes(),16,16,True);pix=[(a if k else 0)|b for a,b,k in zip(pix,col,mask)]
 items.append(('tile',i,16,16,pix))
def graphic(f,ch,pad=False):
 if ch>=1810:fw=fh=16;ox=oy=0
 else:
  mm=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=mm[0]*8,mm[1];ox,oy=mm[2]//16,mm[3]//16
 if pad:ox+=24;oy+=4
 if f==34:ox=oy=0
 w=max(defs[f]['width'],ox+fw);h=max(defs[f]['height'],oy+fh)
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True)
 pts=[(i%fw+ox,i//fw+oy) for i,v in enumerate(mask) if not v];l=min(x for x,y in pts)//2*2;t=min(y for x,y in pts);r=(max(x for x,y in pts)+2)//2*2;b=max(y for x,y in pts)+1
 defs[f]={'chunk':ch,'width':w,'height':h,'crop':[l,t,r-l,b-t]}
 for part in ('mask','color'):
  pix=[]
  for y in range(t,b):
   for x in range(l,r):
    inside=ox<=x<ox+fw and oy<=y<oy+fh;k=(y-oy)*fw+x-ox
    pix.append((15 if not inside or mask[k] else 0) if part=='mask' else (col[k] if inside else 0))
  items.append((part,f,r-l,b-t,pix))
for f in (26,27,32,33,34):graphic(f,defs[f]['chunk'])
for f in range(94,101):graphic(f,172+(f-94)*2)
for f,ch in enumerate(range(295,309),152):graphic(f,ch,True)
for f,ch in enumerate(range(321,329),116):graphic(f,ch,True)
for f,ch in enumerate(range(281,293),124):
 if f not in (128,129,134,135):graphic(f,ch,True)

fgids={};fglookup=[]
for bg,fg in keys:
 if fg and mh[3382+5*1206+fg]&128:
  if fg not in fgids:
   f=46+len(fgids);assert f<84;fgids[fg]=f;graphic(f,1810+fg)
  fglookup.append(fgids[fg])
 else:fglookup.append(0)
for f,ch in enumerate(range(271,279),108):defs[f]={'chunk':ch,'width':64,'height':20,'crop':[24,4,40,16],'mask':[0,0],'color':[0,0],'streamed_aspar':True}
for f,ch in ((128,285),(129,286),(134,291),(135,292)):defs[f]={'chunk':ch,'width':72,'height':36,'crop':[8,4,64,32],'mask':[0,0],'color':[0,0],'streamed_tater':True}
for f,ch,w,h in ((86,197,16,8),(87,198,16,8),(88,211,16,64),(105,210,8,16)):defs[f]={'chunk':ch,'width':w,'height':h,'crop':[0,0,w,h],'mask':[0,0],'color':[0,0],'streamed_object':True}
compressed=[];rawall=bytearray()
for f,ch in enumerate(range(241,257),136):
 mm=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=mm[0]*8,mm[1];ox,oy=mm[2]//16+24,mm[3]//16+4
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True);raw=[]
 for y in range(fh):
  # One-pass TIMP row (fw/2 bytes): 0 = transparent, colour index, or 9 (the second black) for opaque black.
  assert all(not(m and c) for m,c in zip(mask[y*fw:(y+1)*fw],col[y*fw:(y+1)*fw])) and all(c!=9 for m,c in zip(mask[y*fw:(y+1)*fw],col[y*fw:(y+1)*fw]) if not m)
  pix=[0 if m else (c or 9) for m,c in zip(mask,col)];raw.extend(pix[y*fw+x]*16+pix[y*fw+x+1] for x in range(0,fw,2))
 data=[];i=0
 while i<len(raw):
  j=i+1
  while j<len(raw) and raw[j]==raw[i] and j-i<128:j+=1
  if j-i>=3:data.extend((j-i-1,raw[i]));i=j;continue
  j=i
  while j<len(raw) and j-i<128:
   k=j+1
   while k<len(raw) and raw[k]==raw[j] and k-j<3:k+=1
   if k-j>=3:break
   j+=1
  data.append(128|(j-i-1));data.extend(raw[i:j]);i=j

 defs[f]=dict(chunk=ch,width=ox+fw,height=oy+fh,crop=[ox,oy,fw,fh],mask=[0,0],color=[0,0],streamed_brocco=True,stream_offset=len(rawall))
 rawall.extend(raw);compressed.append(data)
print('Brocco compressed bytes',sum(map(len,compressed)),flush=True)
(O/'brocco_attack.bin').write_bytes(rawall)
(R/'src/brocco_images.h').write_text('// Original Brocco frames, PackBits repeat/literal bytes.\n'+''.join('static const u8 brocco%d[]={%s};\n'%(i,','.join(map(str,a))) for i,a in enumerate(compressed))+'static const u8* const brocco_data[16]={'+','.join('brocco%d'%i for i in range(16))+'};\n')
atlas=bytearray(65536);positions,_=place(items,[(0,96,256,400),(48,496,208,16)],atlas,0)
atlas[:8192]=(R/'assets/scene/hero_resident.bin').read_bytes()
for part,i,w,h,pix in items:
 x,y=positions[(part,i)]
 for row in range(h):assert atlas[(y+row)*128+x//2:(y+row)*128+(x+w)//2]==bytes((pix[row*w+k]<<4)|pix[row*w+k+1] for k in range(0,w,2)),(part,i,row)
 if part!='tile':
  l,t,_,_=defs[i]['crop'];defs[i][part]=[x-l,y-t]
for target,source in ((28,26),(29,27)):defs[target]=copy.deepcopy(defs[source])
for name in ('aspar.bin','tater_attack.bin','object_197.bin','object_198.bin','object_211.bin','object_210.bin'):
 (O/name).write_bytes((R/'assets/melon'/name).read_bytes())
def pack9(ids):
 out=bytearray((len(ids)*9+7)//8)
 for i,v in enumerate(ids):
  bit=i*9;out[bit//8]|=(v<<(bit&7))&255
  if bit//8+1<len(out):out[bit//8+1]|=v>>(8-(bit&7))
 return out
idx={k:i for i,k in enumerate(keys)};ids=[idx[k] for k in zip(*m['planes'][:2])]
props=[tuple(mh[3382+d*1206+fg] for d in range(4)) for bg,fg in keys];unique=sorted(set(props));assert len(unique)<=32
data=bytearray(1168);data[:len(keys)]=bytes(unique.index(p) for p in props)
for i,v in enumerate(unique):data[352+i*4:356+i*4]=bytes(v)
for i,(_,fg) in enumerate(keys):
 if mh[3382+5*1206+fg]&127==1:data[480+i//8]|=1<<(i&7)
data[524:524+len(keys)]=bytes(fglookup)
pickups=[dict(x=i%m['width']*16,y=i//m['width']*16-(8 if v==30 else 0),amount=1 if v==29 else 5,frame=32 if v==29 else 34) for i,v in enumerate(m['planes'][2]) if v in (29,30)]
for i,o in enumerate(pickups):struct.pack_into('<HHBB',data,876+i*6,o['x'],o['y'],o['amount'],o['frame'])
spawn=next((i%m['width']*16,i//m['width']*16-16,v) for i,v in enumerate(m['planes'][2]) if v in (1,2))
struct.pack_into('<BBHHBB',data,1120,m['width'],m['height'],spawn[0],spawn[1],spawn[2]==2,len(pickups))
(O/'atlas.bin').write_bytes(atlas);(O/'packed_map.bin').write_bytes(pack9(ids));(O/'stage_data.bin').write_bytes(data);(O/'map.bin').write_bytes(struct.pack('<'+'H'*len(ids),*ids))
(O/'pickups.json').write_text(json.dumps(pickups,indent=2));(O/'combat_assets.json').write_text(json.dumps(dict(sprites=defs,tiles=[positions[('tile',i)] for i in range(len(keys))],foreground=fglookup),indent=2))
(O/'stage_manifest.json').write_text(json.dumps(dict(id=5,name=m['name'],width=m['width'],height=m['height'],tiles=len(keys),spawn=spawn,pickups=len(pickups)),indent=2))
spawns=[]
for code in (47,45,57,41,49,32,33):
 spawns.extend((i%m['width'],i//m['width'],code) for i,v in enumerate(m['planes'][2]) if v==code)
assert len(spawns)==12
(R/'src/pass_spawns.h').write_text('// Original object-plane order: Tater, Aspar, Squasher, Tomato, keys, doors, bombs.\nstatic const u8 pass_spawn[12][3]={'+','.join('{%d,%d,%d}'%s for s in spawns)+'};\n')
boxes=[list(struct.unpack_from('<9h',meta,(ch-71)*18)[4:8]) for ch in list(range(271,279))+list(range(321,329))+list(range(281,293))+list(range(241,257))+list(range(295,309))]
(R/'src/pass_boxes.h').write_text('// Original inclusive hitboxes, pixels.\nstatic const i8 pass_boxes[58][4]={'+','.join('{'+','.join(str(v//16) for v in b)+'}' for b in boxes)+'};\n')

# Keep fifth-stage descriptors in unused map RAM. Original four remain fixed.
blob=bytearray(8910);blob[:len(pack9(ids))]=pack9(ids)
for i,d in enumerate(defs):
 l,t,w,h=d['crop'];my=d['mask'][1]+t;cy=d['color'][1]+t
 struct.pack_into('8B',blob,7000+i*8,d['mask'][0]+l,d['color'][0]+l,my&255,cy&255,w,h,l|(128 if my&256 else 0),t|(128 if cy&256 else 0))
tiles=[positions[('tile',i)] for i in range(len(keys))]
blob[8328:8522]=bytes(x for x,y in tiles)
struct.pack_into('<194H',blob,8522,*[512+y for x,y in tiles])
(O/'load_data.bin').write_bytes(blob)
levels=('scene','melon','bridge','rapids');layouts=[json.loads((R/'assets'/l/'combat_assets.json').read_text()) for l in levels]
h=(R/'src/combat_assets.h').read_text().replace('extern const SpriteDef sprites[4][144];','extern const SpriteDef* const sprites[5];').replace('tile_x[4]','tile_x[5]').replace('tile_y[4]','tile_y[5]');(R/'src/combat_assets.h').write_text(h)
c=(R/'src/combat_assets.c').read_text().replace('#include "combat_assets.h"','#include "combat_assets.h"\n#include "world.h"').replace('SpriteDef sprites[4][144]','SpriteDef sprite_defs[4][144]')
c=c.replace('// TILE TABLES','const SpriteDef* const sprites[5]={&sprite_defs[0][0],&sprite_defs[1][0],&sprite_defs[2][0],&sprite_defs[3][0],(const SpriteDef*)&world_map[7000]};\n// TILE TABLES').replace('tile_x[4]={tx0,tx1,tx2,tx3}','tile_x[5]={tx0,tx1,tx2,tx3,world_map+8328}').replace('tile_y[4]={ty0,ty1,ty2,ty3}','tile_y[5]={ty0,ty1,ty2,ty3,(const u16*)&world_map[8522]}');(R/'src/combat_assets.c').write_text(c)
bonuses=[[(i%mm['width']*16,i//mm['width']*16,v-21) for i,v in enumerate(mm['planes'][2]) if 21<=v<=28] for mm in [next(a for a in maps if a['id']==j) for j in (1,2,3,4,5)]]
(R/'src/bonus_data.h').write_text('static const u8 bonus_count[5]={'+','.join(str(len(a)) for a in bonuses)+'};\nstatic const Bonus bonus_data[5][32]={'+','.join('{'+','.join('{%d,%d,%d}'%v for v in a)+'}' for a in bonuses)+'};\n')
print('Parsnip Pass:',len(keys),'tiles;',len(pickups),'flower pickups;',len(bonuses[4]),'bonuses;',spawns)
