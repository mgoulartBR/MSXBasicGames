"""Mapa do MSX DOOM -> chunks Geo3D (<=255 vértices e <=255 faces por RUN).
Convenção (doc geo3d_engine.v): x direita, y cima, z para dentro; face visível
quando cross(V1-V0, V2-V0) aponta para fora."""
import math
MAP=["################","#P....#........#","#.....#..E.....#","#..##.D........#",
"#..L#.#....##..#","#.....#....L#..#","####D###.......#","#......#...E...#",
"#..E...D.......#","#......#..##...#","#...##.#..L#...#","#...L#.D.......#",
"#.....##########","#..E...........#","#..........E...#","################"]
T=64          # unidades por célula
H=64          # altura da parede
CHUNK=8       # células por lado de chunk (8x8 -> 4 RUNs)
solid=lambda x,z: not(0<=z<len(MAP) and 0<=x<len(MAP[0])) or MAP[z][x] in "#LD"

def q14(v): return max(-32768,min(32767,int(round(v*16384))))
def build():
    chunks=[]
    for cz in range(0,len(MAP),CHUNK):
        for cx in range(0,len(MAP[0]),CHUNK):
            verts=[];vi={};faces=[]
            def V(p):
                if p not in vi: vi[p]=len(verts); verts.append(p)
                return vi[p]
            for z in range(cz,cz+CHUNK):
                for x in range(cx,cx+CHUNK):
                    if not solid(x,z): continue
                    x0,x1,z0,z1=x*T,x*T+T,z*T,z*T+T
                    quads=[]
                    if not solid(x,z-1): quads.append([(x1,0,z0),(x0,0,z0),(x0,H,z0),(x1,H,z0)])
                    if not solid(x,z+1): quads.append([(x0,0,z1),(x1,0,z1),(x1,H,z1),(x0,H,z1)])
                    if not solid(x-1,z): quads.append([(x0,0,z0),(x0,0,z1),(x0,H,z1),(x0,H,z0)])
                    if not solid(x+1,z): quads.append([(x1,0,z1),(x1,0,z0),(x1,H,z0),(x1,H,z1)])
                    # normais para fora do sólido; ordem acima verificada pelo teste (cross)
                    for q in quads:
                        a=[q[1][i]-q[0][i] for i in range(3)]; b=[q[2][i]-q[0][i] for i in range(3)]
                        n=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
                        l=math.sqrt(sum(c*c for c in n))
                        faces.append(([V(p) for p in q],tuple(q14(c/l) for c in n),1))
            if faces:
                assert len(verts)<=255 and len(faces)<=255,(len(verts),len(faces))
                chunks.append(dict(cx=cx,cz=cz,verts=verts,faces=faces))
    return chunks
def entities():
    P=None;E=[]
    for z,row in enumerate(MAP):
        for x,c in enumerate(row):
            if c=='P':P=(x*T+T//2,z*T+T//2)
            if c=='E':E.append((x*T+T//2,z*T+T//2))
    return P,E
# Inimigo: octaedro achatado (6 vértices, 8 faces triangulares, 4º índice repetido)
ENEMY_V=[(0,0,0),(0,0,0)]
def enemy_model(r=20,h=44):
    v=[(r,0,0),(0,0,r),(-r,0,0),(0,0,-r),(0,h,0),(0,0,0)]
    f=[]
    for i,(a,b) in enumerate([(0,1),(1,2),(2,3),(3,0)]):
        # lados superiores (apex=4) e inferiores (apex=5)
        f.append(([a,b,4,4],8)); f.append(([b,a,5,5],8))
    out=[]
    for idx,base in f:
        p=[v[i] for i in idx]
        a=[p[1][i]-p[0][i] for i in range(3)]; b=[p[2][i]-p[0][i] for i in range(3)]
        n=(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
        l=math.sqrt(sum(c*c for c in n)) or 1
        out.append((idx,tuple(q14(c/l) for c in n),base))
    return v,out
