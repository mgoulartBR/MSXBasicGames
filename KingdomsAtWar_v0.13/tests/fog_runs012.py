"""Differential tests for coalesced Fog damage/restoration at arbitrary cameras."""
from pathlib import Path
import ctypes as C,tempfile,subprocess,random,json
R=Path(__file__).resolve().parents[1];s=(R/'src/banks/art_s14_b3.c').read_text()
def fn(n):
 a=s.index(n);b=s.index('{',a);i=b+1;d=1
 while d:d+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i].replace('__banked','')
c='''#include <stdint.h>
typedef uint8_t u8;typedef uint16_t u16;typedef int16_t i16;
#define RN 10
#define NONE 255
typedef struct{i16 x,y;u8 w,h;} Rect;
Rect rect_now[RN];u8 fog_changed[2][128],fog_pending[2],restored[1024];u16 cam_x,cam_y;
const u16 render_bit[]={1,2,4,8,16,32,64,128,256,512};
void restore_field(i16 x,i16 y,u16 w,u16 h,u8 p,u16 cx,u16 cy){for(int yy=y/16;yy<(y+h)/16;yy++)for(int xx=x/16;xx<(x+w)/16;xx++)restored[yy*32+xx]++;}
'''+fn('u16 fog_damage(')+'\n'+fn('void fog_apply(')
class Rect(C.Structure):_fields_=[('x',C.c_int16),('y',C.c_int16),('w',C.c_uint8),('h',C.c_uint8)]
rng=random.Random(120012)
with tempfile.TemporaryDirectory()as d:
 p=Path(d);(p/'t.c').write_text(c);subprocess.run(['gcc','-shared','-fPIC','-O2',str(p/'t.c'),'-o',str(p/'t.so')],check=True);lib=C.CDLL(str(p/'t.so'));lib.fog_damage.restype=C.c_uint16
 bits=(C.c_uint8*256).in_dll(lib,'fog_changed');actors=(Rect*10).in_dll(lib,'rect_now');restored=(C.c_uint8*1024).in_dll(lib,'restored')
 for case in range(2000):
  cx=rng.randrange(257);cy=rng.randrange(353);page=case&1;C.c_uint16.in_dll(lib,'cam_x').value=cx;C.c_uint16.in_dll(lib,'cam_y').value=cy
  original=bytes([255 if case<2 else 0 if case<4 else rng.randrange(256)for _ in range(256)]);bits[:]=original;restored[:]=[0]*1024
  for a in actors:a.x=rng.randrange(-32,512);a.y=rng.randrange(-32,512);a.w=rng.randrange(65);a.h=rng.randrange(65)
  expected=0;tiles=[]
  for y in range(cy//16,(cy+159)//16+1):
   for x in range(cx//16,(cx+255)//16+1):
    cell=y*32+x
    if original[page*128+cell//8]&(1<<(cell&7)):
     tiles.append(cell)
     for j,a in enumerate(actors):
      if a.w and a.h and a.x<x*16+16 and a.x+a.w>x*16 and a.y<y*16+16 and a.y+a.h>y*16:expected|=1<<j
  assert lib.fog_damage(page,0)==expected,(case,'damage')
  lib.fog_apply(page);remaining=bytearray(original)
  for cell in tiles:remaining[page*128+cell//8]&=~(1<<(cell&7))
  assert bytes(bits)==remaining,(case,'clear outside camera')
  assert {i for i,v in enumerate(restored)if v}==set(tiles) and max(restored)<=1,(case,'restore coverage')
report={'passed':True,'random_and_edge_cases':2000,'checks_per_case':['damage mask','exact restore coverage','no duplicate restore','preserve off-camera bits']};(R/'tests/fog_runs012.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',report)
