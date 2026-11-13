#!/usr/bin/env python3
"""Integra os predios do reino inimigo (Mortos) como tiles extras da camada B.
uso: integrate_buildings.py dir  (le dir/0_idx.npy .. dir/7_idx.npy, um por tipo de predio: castle,house,barracks,sawmill,forge,stable,tower,siege)
- recorta cada imagem 64x64 (48x48 para a casa) em tiles de 16x16, deduplica tiles iguais (o tile vazio e sempre o 0)
- grava os tiles (LZ, 128 B cada, transparente = 0) no banco 22 e a tabela predio+deslocamento -> tile em art07.h
Patterns do jogo: tiles inimigos ficam a partir do pattern ENEMY_PAT (depois do bloco 'dither'), sem versao 'dither'."""
import re,sys
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1];d=Path(sys.argv[1])
src=(r/'tools/prepare09.py').read_text()
ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns);lz=ns['lz']
def pack(a):return bytes((a[:,::2]*16+a[:,1::2]).flatten())
sizes=[4,3,4,4,4,4,4,4];tiles=[np.zeros((16,16),np.uint8)];keys={tiles[0].tobytes():0};emap=[0]*128
for t in range(8):
    a=np.load(d/f'{t}_idx.npy');n=sizes[t]
    for ty in range(n):
        for tx in range(n):
            blk=np.where(a[ty*16:(ty+1)*16,tx*16:(tx+1)*16]>=0,a[ty*16:(ty+1)*16,tx*16:(tx+1)*16],0).astype(np.uint8)
            k=blk.tobytes()
            if k not in keys:keys[k]=len(tiles);tiles.append(blk)
            emap[t*16+ty*n+tx]=keys[k]
# Canteiro de obras dos mortos (opcional): site_idx.npy 64x64 (predios 4x4) e site3_idx.npy 48x48 (casa); entradas 128..159
# do mapa; 255 = usar o canteiro dos humanos.
emap+=[255]*32
for name,n,base in (('site_idx.npy',4,128),('site3_idx.npy',3,144)):
    if not (d/name).exists():continue
    a=np.load(d/name);assert a.shape==(16*n,16*n),(name,a.shape)
    for ty in range(n):
        for tx in range(n):
            blk=np.where(a[ty*16:(ty+1)*16,tx*16:(tx+1)*16]>=0,a[ty*16:(ty+1)*16,tx*16:(tx+1)*16],0).astype(np.uint8);k=blk.tobytes()
            if k not in keys:keys[k]=len(tiles);tiles.append(blk)
            emap[base+ty*n+tx]=keys[k]
# Os tiles saem em ate dois bancos de 8 KB (22 e 24): corte guloso pelo tamanho comprimido.
def chunk_enc(ts):return lz(b''.join(pack(t) for t in ts))
k=len(tiles)
while len(chunk_enc(tiles[:k]))>8192:k-=1
first=tiles[:k];second=tiles[k:]
enc1=chunk_enc(first);enc2=chunk_enc(second) if second else b''
assert len(enc2)<=8192,len(enc2)
def cfile(n,body):
    (r/'src/banks'/f'art_s{n}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%n+'\n'.join(','.join(map(str,(body or bytes([0]))[i:i+32]))+',' for i in range(0,len(body or bytes([0])),32))+'\n};\n')
cfile(22,enc1);cfile(24,enc2)
t=(r/'src/art07.h').read_text()
for pat in (r'const Asset enemy_tiles_asset2?=.*?;\n',r'const u8 enemy_tile_map\[\]=.*?;\n',r'#define ENEMY_TILES1? \d+\n'):t=re.sub(pat,'',t)
t+='const Asset enemy_tiles_asset={22,0xA000};\nconst Asset enemy_tiles_asset2={24,0xA000};\nconst u8 enemy_tile_map[]={'+','.join(map(str,emap))+'};\n#define ENEMY_TILES %d\n#define ENEMY_TILES1 %d\n'%(len(tiles),len(first))
(r/'src/art07.h').write_text(t);enc=enc1+enc2;print('bancos: 22 =',len(enc1),'B (',len(first),'tiles ), 24 =',len(enc2),'B (',len(second),'tiles )')
print('tiles unicos',len(tiles))
