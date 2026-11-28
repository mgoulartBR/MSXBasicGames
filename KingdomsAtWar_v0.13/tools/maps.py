#!/usr/bin/env python3
"""Mapas de campanha (64x64 e 128x128): gera terreno, arte e estoque, o caminho das ondas inimigas e grava tudo em bancos de ROM.
Terreno: 0 grama, 1 arvore (estoque 32), 2 agua, 3 ponte/vau, 4 mina 2x2 (estoque 240), 5 castelo humano (4x4), 11 fortaleza inimiga (4x4).
Cada camada (terreno, arte, estoque) sai em blocos de 16 linhas comprimidos (LZ do jogo), empacotados em bancos a partir do 56.
Gera src/map_data.h (cabecalhos, rotas e tabela de blocos) e src/banks/art_s<N>_b3.c.
O mapa 0 do jogo e o mapa embutido de 32x32 (init_world); estes comecam no indice 1."""
import random,re,sys
from collections import deque
from pathlib import Path
r=Path(__file__).resolve().parents[1]
src=(r/'tools/prepare09.py').read_text();ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns);lz=ns['lz']
def pack(a):return bytes((a[:,::2]*16+a[:,1::2]).flatten()) if False else None
FIRST_BANK=56;ROWS=16
# nome, largura, altura, semente, base humana, base inimiga, rio, lagos, florestas, minas extras
MAPS=[   # indice do mapa = numero da missao (1..8)
 dict(w=64,h=64,seed=6,pb=(8,50),eb=(50,14),river=False,lakes=4,forests=12,mines=2),
 dict(w=64,h=64,seed=2,pb=(5,50),eb=(46,18),river=True,lakes=5,forests=14,mines=3),
 dict(w=64,h=64,seed=3,pb=(52,48),eb=(8,18),river=False,lakes=7,forests=16,mines=3),
 dict(w=64,h=64,seed=7,pb=(28,50),eb=(30,16),river=True,lakes=6,forests=15,mines=3),
 dict(w=128,h=128,seed=4,pb=(6,112),eb=(108,22),river=True,lakes=14,forests=40,mines=8),
 dict(w=128,h=128,seed=8,pb=(60,112),eb=(66,20),river=True,lakes=16,forests=42,mines=8),
 dict(w=128,h=128,seed=9,pb=(12,16),eb=(112,104),river=True,lakes=16,forests=44,mines=9),
 dict(w=128,h=128,seed=5,pb=(112,108),eb=(10,26),river=True,lakes=18,forests=46,mines=9),
]
def generate(c):
    rnd=random.Random(c['seed']);w,h=c['w'],c['h'];T=[[0]*w for _ in range(h)]
    pbx,pby=c['pb'];ebx,eby=c['eb']
    def clear(x0,y0,x1,y1):
        for y in range(max(0,y0),min(h,y1)):
            for x in range(max(0,x0),min(w,x1)):T[y][x]=0
    protect=[(pbx-6,pby-8,pbx+14,pby+10),(ebx-7,eby-15,ebx+12,eby+8)]
    def inside(x,y):return any(a<=x<b and c_<=y<d for a,c_,b,d in protect)
    # lagos
    for _ in range(c['lakes']):
        for _try in range(30):
            cx,cy=rnd.randrange(8,w-8),rnd.randrange(8,h-8);rx,ry=rnd.randrange(3,8),rnd.randrange(3,7)
            if any(a-rx-2<=cx<b+rx+2 and c_-ry-2<=cy<d+ry+2 for a,c_,b,d in protect):continue
            for y in range(cy-ry-1,cy+ry+2):
                for x in range(cx-rx-1,cx+rx+2):
                    if 0<=x<w and 0<=y<h and ((x-cx)/rx)**2+((y-cy)/ry)**2<1+rnd.uniform(-.25,.25):T[y][x]=2
            break
    # rio: de cima a baixo, ondulado, com 3 vaus
    if c['river']:
        x0=w//2+rnd.randrange(-w//8,w//8);fords=sorted(rnd.sample(range(h//6,h-h//6),3));ph=rnd.uniform(0,6.28)
        import math
        for y in range(h):
            x=int(x0+math.sin(y/9.0+ph)*w//10)
            for dx in (0,1):
                if 0<=x+dx<w and not inside(x+dx,y):T[y][x+dx]=2
        for fy in fords:
            for y in range(fy-1,fy+2):
                for x in range(w):
                    if T[y][x]==2 and abs(y-fy)<=1:T[y][x]=3
    # florestas
    for _ in range(c['forests']):
        cx,cy=rnd.randrange(w),rnd.randrange(h);rad=rnd.randrange(3,9);dens=rnd.uniform(.5,.85)
        for y in range(cy-rad,cy+rad+1):
            for x in range(cx-rad,cx+rad+1):
                if 0<=x<w and 0<=y<h and T[y][x]==0 and (x-cx)**2+(y-cy)**2<=rad*rad and rnd.random()<dens:T[y][x]=1
    # minas extras (2x2) longe das bases
    for _ in range(c['mines']):
        for _try in range(50):
            x,y=rnd.randrange(2,w-4),rnd.randrange(2,h-4)
            if inside(x,y) or inside(x+1,y+1):continue
            if all(T[yy][xx]==0 for yy in range(y-1,y+3) for xx in range(x-1,x+3)):
                for yy in (y,y+1):
                    for xx in (x,x+1):T[yy][xx]=4
                break
    # bases: limpa, castelo (5), minas e floresta de apoio perto da humana, fortaleza (11)
    clear(pbx-5,pby-7,pbx+13,pby+9);clear(ebx-6,eby-14,ebx+11,eby+7)
    for yy in range(pby,pby+4):
        for xx in range(pbx,pbx+4):T[yy][xx]=5
    for ox,oy in ((7,1),(9,5)):
        for yy in (pby+oy,pby+oy+1):
            for xx in (pbx+ox,pbx+ox+1):T[yy][xx]=4
    for k in range(10):                              # arvores de apoio ao redor da base humana
        ang=rnd.uniform(0,6.283);rr=rnd.uniform(6,10)
        import math
        tx,ty=int(pbx+2+math.cos(ang)*rr),int(pby+2+math.sin(ang)*rr)
        if 0<=tx<w and 0<=ty<h and T[ty][tx]==0:T[ty][tx]=1
    for yy in range(eby,eby+4):
        for xx in range(ebx,ebx+4):T[yy][xx]=11
    # conectividade: da frente do castelo ao portao da base inimiga
    start=(pbx+1,pby+5);goal=(ebx+1,eby-5)
    sites=set()
    for ddx,ddy in zip((4,-3,-3,5,4,-3,-3,5),(-3,-8,-3,-8,1,-12,1,-12)):
        for yy in range(eby+ddy,eby+ddy+4):
            for xx in range(ebx+ddx,ebx+ddx+4):sites.add((xx,yy))
    def bfs(a,b):
        prev={a:None};q=deque([a])
        while q:
            p=q.popleft()
            if p==b:break
            for dx,dy in ((1,0),(0,1),(-1,0),(0,-1)):
                n=(p[0]+dx,p[1]+dy)
                if 0<=n[0]<w and 0<=n[1]<h and n not in prev and n not in sites and T[n[1]][n[0]] in (0,3):prev[n]=p;q.append(n)
        return prev
    prev=bfs(start,goal)
    if goal not in prev:                              # abre um corredor em L
        x,y=start
        while x!=goal[0]:
            x+=1 if goal[0]>x else -1
            if T[y][x] in (1,2):T[y][x]=3 if T[y][x]==2 else 0
        while y!=goal[1]:
            y+=1 if goal[1]>y else -1
            if T[y][x] in (1,2):T[y][x]=3 if T[y][x]==2 else 0
        prev=bfs(start,goal);assert goal in prev
    path=[];p=goal
    while p:path.append(p);p=prev[p]
    path=path[::-1]     # do castelo humano ao portao inimigo
    step=2 if len(path)<=250 else 3
    route=path[::-1][::step]  # do portao inimigo para o castelo, um ponto a cada 2-3 celulas
    route=[p for p in route][1:]+[start]
    assert len(route)<=127,len(route)
    return T,route
def art_layer(T,w,h):
    A=[[0]*w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            t=T[y][x];v=(x*13+y*7)&7;mask=0
            if t>=10 or t==5:A[y][x]=0;continue            # predios: arte calculada no jogo (art_index)
            if t==4:
                dx=1 if x>0 and T[y][x-1]==4 else 0;dy=1 if y>0 and T[y-1][x]==4 else 0;A[y][x]=25+dy*2+dx;continue
            if t==2:
                if x and T[y][x-1]!=2:A[y][x]=10
                elif x<w-1 and T[y][x+1]!=2:A[y][x]=11
                elif y and T[y-1][x]!=2:A[y][x]=12
                elif y<h-1 and T[y+1][x]!=2:A[y][x]=13
                else:A[y][x]=2
                continue
            if t==3:A[y][x]=3;continue
            if x and T[y][x-1]==1:mask|=1
            if t==1:mask|=2
            if x and y<h-1 and T[y+1][x-1]==1:mask|=4
            if y<h-1 and T[y+1][x]==1:mask|=8
            A[y][x]=29+((x+y)&1)*15+mask-1 if mask else (0 if v<4 else 6 if v==4 else 7 if v==5 else 14 if v==6 else 15)
    return A
def stock_layer(T,w,h):
    return [[32 if T[y][x]==1 else 240 if (T[y][x]==4 and (x==0 or T[y][x-1]!=4) and (y==0 or T[y-1][x]!=4)) else 0 for x in range(w)] for y in range(h)]
def main():
    banks=[bytearray()];tables=[];hdr=[];routes=bytearray();chunk_tab=bytearray()
    def add(blob):
        assert len(blob)<=8192
        if len(banks[-1])+len(blob)>8192:banks.append(bytearray())
        ref=(FIRST_BANK+len(banks)-1,0xA000+len(banks[-1]));banks[-1]+=blob;return ref
    for m,c in enumerate(MAPS,1):
        w,h=c['w'],c['h'];T,route=generate(c);A=art_layer(T,w,h);S=stock_layer(T,w,h)
        # os blocos guardam os bytes da camada (w por linha), em linhas de 16
        refs=[]
        for layer in (T,A,S):
            for y0 in range(0,h,ROWS):
                rows=layer[y0:y0+ROWS];data=bytes(v for row in rows for v in row)
                # lz() do jogo trabalha em bytes (comprime qualquer sequencia)
                refs.append(add(lz(data)))
        chunks_per_layer=(h+ROWS-1)//ROWS
        ro=len(routes);routes+=bytes(v for p in route for v in p)
        co=len(chunk_tab);chunk_tab+=b''.join(bytes((b,p&255,p>>8)) for b,p in refs)
        hdr.append((w,h,c['pb'][0],c['pb'][1],c['eb'][0],c['eb'][1],len(route),chunks_per_layer,ro,co))
        print('mapa',m,w,'x',h,'rota',len(route),'blocos',len(refs))
    for i,b in enumerate(banks):
        body=bytes(b);(r/'src/banks'/f'art_s{FIRST_BANK+i}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%(FIRST_BANK+i)+'\n'.join(','.join(map(str,body[j:j+32]))+',' for j in range(0,len(body),32))+'\n};\n')
    out='/* gerado por tools/maps.py */\n#define MAP_COUNT %d\n#define MAP_ROWS %d\n'%(len(MAPS)+1,ROWS)
    flat=[]
    for h_ in hdr:flat+=list(h_[:8])+[h_[8]&255,h_[8]>>8,h_[9]&255,h_[9]>>8]
    out+='static const u8 map_hdr[]={%s};\n'%','.join(map(str,flat))   # 12 bytes por mapa (indice 1..)
    out+='static const u8 map_route[]={%s};\n'%','.join(map(str,routes or b'\0'))
    out+='static const u8 map_chunks[]={%s};\n'%','.join(map(str,chunk_tab or b'\0'))
    (r/'src/map_data.h').write_text(out)
    print('bancos',FIRST_BANK,'..',FIRST_BANK+len(banks)-1,[len(b) for b in banks])
if __name__=='__main__':main()
