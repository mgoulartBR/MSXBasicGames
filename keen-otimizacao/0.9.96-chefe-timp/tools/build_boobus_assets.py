#!/usr/bin/env python3
"""Build tenth playable stage from the original map and EGA data, preserving stages 1..5."""
from pathlib import Path
import ast,copy,json,struct
import numpy as np
from import_ega import planar
R=Path(__file__).resolve().parents[1];G=R/'assets/generated';O=R/'assets/boobus';O.mkdir(exist_ok=True)
maps=json.loads((G/'maps.json').read_text());m=next(a for a in maps if a['id']==15)
meta=(G/'chunk_0002.bin').read_bytes();mh=(G/'MAPHEAD.KDR').read_bytes()
node=next(n for n in ast.parse((R/'tools/build_stages.py').read_text().replace('if stage==2:free=[(fx,max(fy,yy),fw,fy+fh-max(fy,yy)) for fx,fy,fw,fh in free if fy+fh>yy]','pass')).body if isinstance(n,ast.FunctionDef) and n.name=='place')
exec(compile(ast.Module(body=[node],type_ignores=[]),'packer','exec'))
keys=sorted(set(zip(*m['planes'][:2])));items=[];defs=copy.deepcopy(json.loads((R/'assets/pass/combat_assets.json').read_text())['sprites'])
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
for f in range(102,105):graphic(f,193+f-102)
for f in range(108,117):
 ch=359+f-108;mm=struct.unpack_from('<9h',meta,(ch-71)*18);fw,fh=mm[0]*8,mm[1];ox,oy=24+mm[2]//16,4+mm[3]//16
 col,mask=planar((G/f'chunk_{ch:04}.bin').read_bytes(),fw,fh,True)
 pts=[(i%fw,i//fw)for i,v in enumerate(mask)if not v];l=min(x for x,y in pts)//2*2;t=min(y for x,y in pts);rr=(max(x for x,y in pts)+2)//2*2;bb=max(y for x,y in pts)+1;w,h=rr-l,bb-t
 raw=bytearray()
 # One-pass TIMP image per row (w/2 bytes): 0 = transparent, colour index, or 9 (the second black) for opaque black.
 for y in range(t,bb):
  vals=[0 if mask[y*fw+x] else (col[y*fw+x] or 9) for x in range(l,rr)]
  assert 9 not in [col[y*fw+x] for x in range(l,rr) if not mask[y*fw+x]]
  raw.extend((vals[x]<<4)|vals[x+1]for x in range(0,w,2))
 (O/f'boss_{f-108}.bin').write_bytes(raw)
 defs[f]=dict(chunk=ch,width=ox+fw,height=oy+fh,crop=[ox+l,oy+t,w,h],mask=[0,0],color=[0,0],streamed_boobus=True)

for f in range(117,119):graphic(f,195+f-117)
for f in range(119,123):graphic(f,168+f-119)
fgids={};fglookup=[]
for bg,fg in keys:
 if fg and mh[3382+5*1206+fg]&128:
  if fg not in fgids:
   f=(list(range(46,84))+list(range(90,94)))[len(fgids)];fgids[fg]=f;graphic(f,1810+fg)
  fglookup.append(fgids[fg])
 else:fglookup.append(0)
# Identical masks and composed tiles share atlas storage.
unique_items=[];aliases={};seen={}
for item in items:
 part,i,w,h,pix=item;k=(w,h,bytes(pix))
 if k in seen:aliases[(part,i)]=seen[k]
 else:seen[k]=(part,i);unique_items.append(item)
print('Boobus atlas payload',sum(w*h//2 for _,_,w,h,_ in items),'deduplicated',sum(w*h//2 for _,_,w,h,_ in unique_items),flush=True)
atlas=bytearray(65536);positions,_=place(unique_items,[(0,128,256,368),(48,496,208,16)],atlas,0)
for k,v in aliases.items():positions[k]=positions[v]
# y0..63: resident hero; y64..95: hero dynamic tables; y96..127: boss strips.
assert not any(atlas[:128*128])
atlas[:8192]=(R/'assets/scene/hero_resident.bin').read_bytes()
for part,i,w,h,pix in items:
 x,y=positions[(part,i)]
 for row in range(h):assert atlas[(y+row)*128+x//2:(y+row)*128+(x+w)//2]==bytes((pix[row*w+k]<<4)|pix[row*w+k+1] for k in range(0,w,2)),(part,i,row)
 if part!='tile':
  l,t,_,_=defs[i]['crop'];defs[i][part]=[x-l,y-t]
for target,source in ((28,26),(29,27)):defs[target]=copy.deepcopy(defs[source])
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
(O/'stage_manifest.json').write_text(json.dumps(dict(id=15,campaign_id=11,name=m['name'],width=m['width'],height=m['height'],tiles=len(keys),spawn=spawn,pickups=len(pickups)),indent=2))
spawns=[(i%m['width'],i//m['width'],v) for i,v in enumerate(m['planes'][2]) if v==61]
assert spawns==[(21,26,61)]
(R/'src/boobus_spawns.h').write_text('static const u8 boobus_spawn[3]={21,26,61};\n')
blob=bytearray(6830);packed=pack9(ids);assert len(packed)==1182;blob[:len(packed)]=packed
for i,d in enumerate(defs):
 l,t,w,h=d['crop'];my=d['mask'][1]+t;cy=d['color'][1]+t
 struct.pack_into('8B',blob,5000+i*8,d['mask'][0]+l,d['color'][0]+l,my&255,cy&255,w,h,l|(128 if my&256 else 0),t|(128 if cy&256 else 0))
tiles=[positions[('tile',i)]for i in range(len(keys))];assert len(tiles)==146
blob[6328:6474]=bytes(x for x,y in tiles);struct.pack_into('<146H',blob,6496,*[512+y for x,y in tiles]);(O/'load_data.bin').write_bytes(blob)
c=(R/'src/combat_assets.c').read_text().replace('sprites[10]','sprites[11]').replace('(const SpriteDef*)&world_map[5000]}','(const SpriteDef*)&world_map[5000],(const SpriteDef*)&world_map[5000]}').replace('tile_x[10]','tile_x[11]').replace('world_map+6328}','world_map+6328,world_map+6328}').replace('tile_y[10]','tile_y[11]').replace('(const u16*)&world_map[6496]}','(const u16*)&world_map[6496],(const u16*)&world_map[6496]}');(R/'src/combat_assets.c').write_text(c)
h=(R/'src/combat_assets.h').read_text().replace('[10]','[11]');(R/'src/combat_assets.h').write_text(h)
bonuses=[[(i%mm['width']*16,i//mm['width']*16,v-21)for i,v in enumerate(mm['planes'][2])if 21<=v<=28]for mm in [next(a for a in maps if a['id']==j)for j in (1,2,3,4,5,7,9,10,12,14,15)]]
(R/'src/bonus_data.h').write_text('static const u8 bonus_count[11]={'+','.join(str(len(a))for a in bonuses[:10])+'};\nstatic const Bonus bonus_data[10][40]={'+','.join('{'+','.join('{%d,%d,%d}'%v for v in a)+('' if a else '{0,0,0}')+'}'for a in bonuses[:10])+'};\n')
print('Boobus Room:',len(keys),'tiles;',len(pickups),'seed pickups;',len(bonuses[10]),'bonuses; one boss')
