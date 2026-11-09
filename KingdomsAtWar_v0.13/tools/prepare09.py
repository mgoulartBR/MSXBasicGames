"""Compile approved art for the gameplay renderers; ASCII8 data windows."""
from pathlib import Path
from PIL import Image
import numpy as np,json,re
r=Path(__file__).resolve().parents[1];out=r/'art07/runtime';out.mkdir(exist_ok=True)
s=(r/'src/assets.h').read_text();oldpal=list(map(int,re.search(r'const u8 palette68\[\] = \{(.*?)\};',s,re.S)[1].replace('\n','').strip().strip(',').split(',')))
levels=[(0,0,0),(1,1,1),(1,2,1),(2,3,2),(1,1,2),(2,3,4),(2,1,1),(4,2,1),(3,2,1),(6,4,2),(5,4,2),(6,6,4),(2,2,2),(4,4,4),(6,6,6),(7,7,6)]
pal=[round(c*255/7) for row in levels for c in row]+[0]*720
remap=np.array([0,1,2,3,4,5,6,5,7,9,8,11,2,3,13,14],dtype='uint8')
def expand(b,w=256):
 a=np.frombuffer(b,dtype='uint8').reshape(-1,w//2);out=np.empty((len(a),w),dtype='uint8');out[:,::2]=a>>4;out[:,1::2]=a&15;return out
def pack(a):return bytes((a[:,::2]*16+a[:,1::2]).flatten())
def png(a,p):
 im=Image.fromarray(a,'P');im.putpalette(pal);im.info['transparency']=0;im.save(p)
def lz(a):
 a=list(a);out=[];i=0
 while i<len(a):
  pos=len(out);out.append(0);flag=0
  for bit in range(8):
   if i>=len(a):break
   best=0;bd=0
   for d in range(1,min(256,i)+1):
    n=0
    while n<min(255,len(a)-i) and a[i+n]==a[i+n-d]:n+=1
    if n>best:best=n;bd=d
   if best>=3:out.extend([bd-1,best-3]);i+=best
   else:flag|=1<<bit;out.append(a[i]);i+=1
  out[pos]=flag
 return bytes(out)
chunks=[bytearray()];records=[]
def add(name,data):
 c=lz(data);assert len(c)<8192
 if len(chunks[-1])+len(c)>8192:chunks.append(bytearray())
 bank=3+len(chunks);offset=len(chunks[-1]);chunks[-1].extend(c);records.append(dict(name=name,bank=bank,offset=offset,compressed=len(c),raw=len(data)));(out/(name+'.bin')).write_bytes(data)
 return '{%d,0x%04X}'%(bank,0xA000+offset)
terrain=remap[expand((r/'art/reference_v68_atlas.bin').read_bytes())];tiles=[];mapping=[0]*256
for t in list(range(16))+list(range(25,59)):
 mapping[t]=len(tiles);tiles.append(terrain[(t>>4)*16:(t>>4)*16+16,(t&15)*16:(t&15)*16+16].copy())
names=['castle','house','barracks','sawmill','forge','stable','tower','siege'];base=[];semantic=64
for name in names:
 base.append(semantic);a=np.array(Image.open(r/'art07/native/kingdom'/f'{name}.png'));n=a.shape[0]//16
 for y in range(n):
  for x in range(n):mapping[semantic]=len(tiles);tiles.append(a[y*16:y*16+16,x*16:x*16+16]);semantic+=1
assert semantic==185 and len(tiles)==171
# Construction stakes, stone footings and beams are native state graphics.
site=np.zeros((64,64),dtype='uint8')
for y in range(12,60):
 for x in range(5,59):
  if (x*7+y*11)%11<4:site[y,x]=8
for y in [14,55]:site[y:y+2,6:59]=11;site[y+2:y+4,6:59]=7
for x in [7,55]:site[13:58,x:x+2]=7;site[13:58,x+2:x+3]=9
for x,y in [(5,10),(54,10),(5,52),(54,52)]:site[y:y+10,x:x+3]=7;site[y:y+2,x:x+3]=11
for x in [16,24,32,40]:site[28:32,x:x+14]=7;site[28,x:x+14]=9
for y in range(4):
 for x in range(4):mapping[semantic]=len(tiles);tiles.append(site[y*16:y*16+16,x*16:x*16+16]);semantic+=1
while len(tiles)<192:tiles.append(np.full((16,16),3,dtype='uint8'))
flat=b''.join(pack(a) for a in tiles);atlas=np.zeros((192,256),dtype='uint8')
for t,a in enumerate(tiles):atlas[(t>>4)*16:(t>>4)*16+16,(t&15)*16:(t&15)*16+16]=a
png(atlas,out/'static.png');atlasrefs=[add('static'+str(i),flat[i*8192:(i+1)*8192]) for i in range(3)]
font=expand((r/'art/reference_v68_font.bin').read_bytes());font=remap[font];font[font==14]=15;fontref=add('font',pack(font));png(font,out/'font.png')
def pose_cells(bounds,sizes):
    out=[]
    for k in range(168):
        n=sizes[k//24]//16;l,t,w,h=bounds[k*4:k*4+4]
        if w and h:cx0=l>>4;cy0=t>>4;cols=((l+w-1)>>4)-cx0+1;rows=((t+h-1)>>4)-cy0+1
        else:cx0=cy0=0;cols=rows=n
        out+=[cy0*16+cx0,cols*16+rows]
    return out
roles=['peasant','swordsman','spearman','archer','knight','ram','enemy_sword'];sizes=[32,32,48,32,48,48,32];refs=['{0,0}']*168;bounds=[0]*672
# Canvas/cache layout stays stable. Compile the two oversized classes at the
# infantry scale, using one transform for every direction and walking phase.
# Integer nearest sampling preserves palette indices; align the bottom centre
# so selection, depth sorting and the feet's world position stay unchanged.
content_sizes=[32,32,36,32,40,48,32]
def unit_pixels(a,kind):
 side=sizes[kind];target=content_sizes[kind]
 assert a.shape==(side,side) and a.max()<16
 if target==side:return a
 sample=((np.arange(target)*2+1)*side)//(target*2)
 result=np.zeros_like(a);left=(side-target)//2;top=side-target
 result[top:top+target,left:left+target]=a[np.ix_(sample,sample)]
 return result
for kind,role in enumerate(roles):
 for f in range(24 if not kind else 16):
  a=np.array(Image.open(r/'art07/native'/f'frame_{16+f:02d}.png')) if not kind else np.array(Image.open(r/'art07/native/kingdom'/f"{'swordsman' if kind==6 else role}_{f:02d}.png"));n=sizes[kind]//16
  a=unit_pixels(a,kind)
  if kind==6:
   team=np.arange(16,dtype=np.uint8);team[4]=6;team[5]=9;a=team[a]
  raw=b''.join(pack(a[y*16:y*16+16,x*16:x*16+16]) for y in range(n) for x in range(n));key=kind*24+f;refs[key]=add('pose'+str(key),raw);yy,xx=np.nonzero(a);bounds[key*4:key*4+4]=[int(xx.min()),int(yy.min()),int(xx.max()-xx.min()+1),int(yy.max()-yy.min()+1)]
assert len(chunks)+4<=32
for j,data in enumerate(chunks,4):
 (r/'src/banks'/f'art_s{j}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%j+'\n'.join(','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32))+'\n};\n')
def emit(n,a):return 'const u8 '+n+'[]={'+','.join(map(str,a))+'};\n'
text='typedef struct {u8 bank;u16 ptr;} Asset;\nconst Asset static_assets[]={'+','.join(atlasrefs)+'};\nconst Asset font_asset='+fontref+';\nconst Asset poses[]={'+','.join(refs)+'};\n'
text+=emit('build_base',base)+emit('build_sizes',[4,3,4,4,4,4,4,4])+emit('kind_sizes',sizes)+emit('tile_source',mapping)+emit('pose_bounds',bounds)+emit('pose_cells',pose_cells(bounds,sizes))
text+=emit('palette68',[v for red,green,blue in levels for v in [(red<<4)|blue,green]])+emit('palette9',[round(c*31/7) for pix in levels for c in pix])
ring=[0]*128
for y,l,rr in [(10,6,9),(11,3,12),(12,1,14),(13,3,12),(14,6,9)]:
 for x in range(l,rr+1):
  if y in [10,14] or x in [l,rr]:ring[y*8+x//2]|=10 if x&1 else 160
text+=emit('ring_pixels',ring);(r/'src/art07.h').write_text(text)
manifest=dict(mapper='ASCII8',rom_bytes=262144,records=records,banks=len(chunks),tile_count=192,buildings=names,build_sizes=[4,3,4,4,4,4,4,4],units=roles,sizes=sizes,total_poses=120)
manifest['content_sizes']=content_sizes
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print('BANKS',len(chunks),'packed bytes',sum(map(len,chunks)),'frames',120,flush=True)

