"""Gera src/level.h (dados prontos para stream VDATA/FDATA do Geo3D)."""
import math,os,struct,sys
sys.path.insert(0,os.path.dirname(__file__))
import level
def arr(name,data,ctype="const unsigned char"):
    rows=[",".join(str(b) for b in data[i:i+24]) for i in range(0,len(data),24)]
    return f"{ctype} {name}[{len(data)}]={{\n"+",\n".join(rows)+"\n};\n"
def vbytes(vs): return b"".join(struct.pack("<hhh",*v) for v in vs)
def fbytes(fs): return b"".join(struct.pack("<BBBBhhhB",*(f[0]+[0]*0),*f[1],f[2]) if False else
                               struct.pack("<BBBBhhhB",f[0][0],f[0][1],f[0][2],f[0][3],f[1][0],f[1][1],f[1][2],f[2]) for f in fs)
o="/* GERADO por tools/gen_level.py - não editar */\n#define NCHUNK %d\n"
ch=level.build(); out=o%len(ch)
for i,c in enumerate(ch):
    out+=arr(f"ch{i}_v",vbytes(c['verts']))+arr(f"ch{i}_f",fbytes(c['faces']))
out+="typedef struct{const unsigned char*v;const unsigned char*f;unsigned char nv,nf;int cx,cz;}Chunk;\n"
out+="const Chunk chunks[NCHUNK]={\n"+",\n".join(
 f"{{ch{i}_v,ch{i}_f,{len(c['verts'])},{len(c['faces'])},{c['cx']*level.T+level.T*level.CHUNK//2},{c['cz']*level.T+level.T*level.CHUNK//2}}}" for i,c in enumerate(ch))+"};\n"
ev,ef=level.enemy_model()
out+=arr("enemy_v",vbytes(ev))+arr("enemy_f",fbytes(ef))
out+=f"#define ENEMY_NV {len(ev)}\n#define ENEMY_NF {len(ef)}\n"
sin=[int(round(math.sin(i*2*math.pi/256)*16384)) for i in range(256)]
out+="const int sin_tab[256]={"+",".join(map(str,sin))+"};\n"
pal=[(0,0,0),(1,0,0),(2,0,0),(3,0,0),(4,0,1),(5,1,1),(6,1,1),(7,2,2),(1,1,0),(2,1,0),(3,2,0),(4,3,0),(5,3,1),(6,4,1),(7,5,2),(1,1,2)]
pb=[]
for r,g,b in pal: pb+=[(r<<4)|b,g]
out+=arr("palette",pb)
P,E=level.entities()
out+=f"#define START_X {P[0]}\n#define START_Z {P[1]}\n#define NENEMY {len(E)}\n"
out+="const int enemy_start[NENEMY][2]={"+",".join(f"{{{x},{z}}}" for x,z in E)+"};\n"
out+="#define MAPW %d\n#define MAPH %d\n"%(len(level.MAP[0]),len(level.MAP))
out+="const unsigned char map_solid[%d]={"%(len(level.MAP)*len(level.MAP[0]))+",".join("1" if level.solid(x,z) else "0" for z in range(len(level.MAP)) for x in range(len(level.MAP[0])))+"};\n"
open(os.path.join(os.path.dirname(__file__),"..","src","level.h"),"w").write(out)
print("level.h:",len(ch),"chunks")
