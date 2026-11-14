# T esperado calculado do atlas.bin ORIGINAL (build base); comparado com a VRAM da ROM TIMP após o carregamento
import re,sys,numpy as np,json
from pathlib import Path
proj=Path(sys.argv[1]).resolve();baseproj=Path(sys.argv[2]).resolve();lvl=int(sys.argv[3]);timpv=sys.argv[4]
src=open(proj/'tools/build_timp_frames.py').read().split("lists=[];bits=[]")[0].replace("Path(__file__).resolve().parents[1]","proj")
exec(src)
hdr=(proj/'src/timp_frames.h').read_text()
lst=[int(x) for x in re.search(r'timp_list%d\[\]=\{([^}]*)\}'%(lvl-1),hdr)[1].split(',') if x and int(x)!=255]
li=lvl-1;layout=json.loads((root/'assets'/LEVELS[li]/'combat_assets.json').read_text());defs=defs_for(li,len(layout['sprites']))
def nibf(b):
    a=np.frombuffer(b,dtype=np.uint8).reshape(-1,128);px=np.empty((a.shape[0],256),np.uint8);px[:,0::2]=a>>4;px[:,1::2]=a&15;return px
B=nibf((proj/'assets'/LEVELS[li]/'atlas.bin').read_bytes())
T=nibf(np.frombuffer(open(timpv,'rb').read(),dtype=np.uint8)[0x10000:0x20000].tobytes())
bad=0
for f in lst:
    d=defs[f];x,y,w,h=d['mx'],d['my'],d['w'],d['h'];cx,cy=d['cx'],d['cy']
    M=B[y:y+h,x:x+w];C=B[cy:cy+h,cx:cx+w]
    exp=np.where(C!=0,C,np.where(M==0,9,0));got=T[y:y+h,x:x+w]
    if not np.array_equal(exp,got):bad+=1;print(' frame',f,(x,y,w,h),'mismatch',int((exp!=got).sum()),'of',w*h)
print(f'level {lvl}: {len(lst)} converted frames, {bad} mismatching')
