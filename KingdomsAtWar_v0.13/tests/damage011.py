"""Pixel differential test using the actual banked C damage calculation."""
from pathlib import Path
import ctypes as C, tempfile,subprocess,random,json,hashlib
import numpy as np
R=Path(__file__).resolve().parents[1]
def function(s,name):
 a=s.index(name);b=s.index('{',a);i=b+1;d=1
 while d:
  d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i].replace('__banked','')
v=(R/'src/video.h').read_text();b=(R/'src/banks/art_s27_b3.c').read_text()
body=function(b,'u16 damage_mask(')
fog=function((R/'src/banks/art_s14_b3.c').read_text(),'u16 fog_damage(')
arrow=function((R/'src/banks/art_s25_b3.c').read_text(),'u16 arrow_damage(')
c='''#include <stdint.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t i16;
#define RN 10
#define NONE 255
#define PS 16
typedef struct {i16 x,y;u8 w,h;} Rect;
Rect rect_now[RN],rect_old[2][RN];
const u16 render_bit[RN]={1,2,4,8,16,32,64,128,256,512};
u8 box_old[2],changed_x[2][32],changed_y[2][32],changed_count[2],ghost_old[2],ghost_size[2],place_x,place_y,build_kind;
u16 box_l[2],box_t[2],box_r[2],box_b[2],ghost_x[2],ghost_y[2],drag_l,drag_t,drag_r,drag_b;
u16 arrow_old[2],arrow_old_x[2][PS],arrow_old_y[2][PS];
u8 fog_changed[2][128];u16 cam_x,cam_y;
u8 building_size(u8 kind){return 2;}
'''+function(v,'u8 intersect(')+'\n'+function(b,'static u8 box_overlap(')+'\n'+arrow+'\n'+fog+'\n'+body
class Rect(C.Structure):_fields_=[('x',C.c_int16),('y',C.c_int16),('w',C.c_uint8),('h',C.c_uint8)]
rng=random.Random(936284)
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'damage.c').write_text(c);subprocess.run(['gcc','-shared','-fPIC','-O2',str(p/'damage.c'),'-o',str(p/'damage.so')],check=True)
 lib=C.CDLL(str(p/'damage.so'));lib.damage_mask.argtypes=[C.c_uint8,C.c_uint16,C.c_uint8];lib.damage_mask.restype=C.c_uint16
 def arr(name,t,n):return (t*n).in_dll(lib,name)
 def put(name,val,t=C.c_uint8):t.in_dll(lib,name).value=val
 now=arr('rect_now',Rect,10);old=arr('rect_old',Rect,20)
 def border(im,box,fill=None):
  if box is None:return
  l,t,r,b=box
  for ys,xs in [(slice(t,t+1),slice(l,r+1)),(slice(b,b+1),slice(l,r+1)),(slice(t,b+1),slice(l,l+1)),(slice(t,b+1),slice(r,r+1))]:
   im[ys,xs]=15 if fill is None else fill[ys,xs]
 def actor(im,a,key):
  x,y,w,h=a
  if not w:return
  yy,xx=np.indices((h,w));mask=(xx+2*yy+key)%4!=0;pix=im[y:y+h,x:x+w];pix[mask]=4+key%10
 def erase(im,a,bg):
  x,y,w,h=a;im[y:y+h,x:x+w]=bg[y:y+h,x:x+w]
 def full(bg,actors,keys,box,ghost):
  im=bg.copy()
  for a,k in zip(actors,keys):actor(im,a,k)
  border(im,ghost);border(im,box);return im
 operations=0
 for case in range(3000):
  page=case&1
  previous=[(rng.randrange(2,70),rng.randrange(2,70),rng.randrange(4,20),rng.randrange(4,20)) for i in range(10)]
  current=list(previous);keys=[rng.randrange(20) for _ in range(10)];newkeys=keys.copy()
  for i in range(10):
   if rng.random()<.35:
    a=list(current[i]);a[0]=max(0,min(72,a[0]+rng.randrange(-5,6)));a[1]=max(0,min(72,a[1]+rng.randrange(-5,6)));current[i]=tuple(a);newkeys[i]=rng.randrange(20)
   if rng.random()<.05:current[i]=(*current[i][:2],0,current[i][3])
  def box():
   if rng.random()<.3:return None
   x,y=rng.randrange(0,50),rng.randrange(0,50);return (x,y,x+rng.randrange(12,42),y+rng.randrange(12,42))
  arrows=[(rng.randrange(3,90),rng.randrange(3,90),5,5) for _ in range(rng.randrange(4))]
  arr('arrow_old',C.c_uint16,2)[page]=(1<<len(arrows))-1
  for i,(x,y,_,_) in enumerate(arrows):
   arr('arrow_old_x',C.c_uint16,32)[page*16+i]=x+2;arr('arrow_old_y',C.c_uint16,32)[page*16+i]=y+2
  oldbox=box();newbox=box() if rng.random()<.45 else oldbox
  def ghost():
   if rng.random()<.4:return None
   x,y=rng.randrange(0,4)*16,rng.randrange(0,4)*16;return (x,y,x+31,y+31)
  oldghost=ghost();newghost=ghost() if rng.random()<.45 else oldghost
  oldbg=np.ones((96,96),dtype=np.uint8);bg=oldbg.copy();tiles=[]
  for _ in range(rng.randrange(4)):
   x,y=rng.randrange(6),rng.randrange(6);tiles.append((x,y));bg[y*16:y*16+16,x*16:x*16+16]=2+rng.randrange(2)
  initial=sum(1<<i for i in range(10) if previous[i]!=current[i] or keys[i]!=newkeys[i])
  for i in range(10):old[page*10+i]=Rect(*previous[i]);now[i]=Rect(*current[i])
  arr('changed_count',C.c_uint8,2)[page]=len(tiles) if case%2 else 0
  for j in range(256):arr('fog_changed',C.c_uint8,256)[j]=0
  if not case%2:
   for xx,yy in tiles:
    cell=yy*32+xx;arr('fog_changed',C.c_uint8,256)[page*128+(cell>>3)]|=1<<(cell&7)
  for i,(x,y) in enumerate(tiles):arr('changed_x',C.c_uint8,64)[page*32+i]=x;arr('changed_y',C.c_uint8,64)[page*32+i]=y
  arr('box_old',C.c_uint8,2)[page]=oldbox is not None
  for name,val in zip(['box_l','box_t','box_r','box_b'],oldbox or (0,0,0,0)):arr(name,C.c_uint16,2)[page]=val
  for name,val in zip(['drag_l','drag_t','drag_r','drag_b'],newbox or (0,0,0,0)):put(name,val,C.c_uint16)
  arr('ghost_old',C.c_uint8,2)[page]=oldghost is not None;arr('ghost_size',C.c_uint8,2)[page]=32
  arr('ghost_x',C.c_uint16,2)[page]=(oldghost or [0,0])[0];arr('ghost_y',C.c_uint16,2)[page]=(oldghost or [0,0])[1]
  put('place_x',(newghost or [0,0])[0]//16);put('place_y',(newghost or [0,0])[1]//16)
  flags=(1 if newbox else 0)|(2 if oldbox!=newbox else 0)|(4 if newghost else 0)|(8 if oldghost!=newghost else 0)
  mask=lib.damage_mask(page,initial,flags|16)
  im=full(oldbg,previous,keys,oldbox,oldghost)
  for aa in arrows:actor(im,aa,7)
  for aa in arrows:erase(im,aa,bg)
  if oldbox!=newbox:border(im,oldbox,bg)
  if oldghost!=newghost:border(im,oldghost,bg)
  for i in range(10):
   if initial&(1<<i):erase(im,previous[i],bg)
  for x,y in tiles:im[y*16:y*16+16,x*16:x*16+16]=bg[y*16:y*16+16,x*16:x*16+16]
  for i in range(10):
   if mask&(1<<i):actor(im,current[i],newkeys[i])
  border(im,newghost);border(im,newbox)
  expected=full(bg,current,newkeys,newbox,newghost)
  if not np.array_equal(im,expected):raise AssertionError((case,initial,mask,int(np.count_nonzero(im!=expected))))
  operations+=mask.bit_count()
 arr('arrow_old',C.c_uint16,2)[0]=0
 for j in range(256):arr('fog_changed',C.c_uint8,256)[j]=0
 # Explicitly guard against broad invalidation of untouched interiors.
 for i in range(10):now[i]=Rect(20,20,0,4);old[i]=Rect(20,20,0,4)
 now[0]=Rect(20,20,4,4);old[0]=Rect(20,20,4,4)
 arr('changed_count',C.c_uint8,2)[0]=0;arr('box_old',C.c_uint8,2)[0]=0
 for name,value in [('drag_l',0),('drag_t',0),('drag_r',60),('drag_b',60)]:put(name,value,C.c_uint16)
 assert lib.damage_mask(0,0,3)==0,'box interior must remain untouched'
 arr('changed_count',C.c_uint8,2)[0]=1
 arr('changed_x',C.c_uint8,64)[0]=5;arr('changed_y',C.c_uint8,64)[0]=5
 assert lib.damage_mask(0,0,0)==0,'distant terrain must not invalidate actors'
 arr('changed_x',C.c_uint8,64)[0]=1;arr('changed_y',C.c_uint8,64)[0]=1
 assert lib.damage_mask(0,0,0)==1,'changed terrain must redraw intersecting actor'
 result=dict(passed=True,cases=3000,pixels_per_case=96*96,comparisons=3000*96*96,fog_damage_sha256=hashlib.sha256(fog.encode()).hexdigest(),projectile_damage_sha256=hashlib.sha256(arrow.encode()).hexdigest(),actual_damage_c_sha256=hashlib.sha256(body.encode()).hexdigest(),redrawn_actors=operations,targeted_checks=3)
 (R/'tests/damage011.json').write_text(json.dumps(result,indent=2)+'\n');print('PASS',result)
