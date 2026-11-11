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
raw=b''.join(pack(t) for t in tiles);enc=lz(raw)
assert len(enc)<=8192,len(enc)
body=enc
(r/'src/banks/art_s22_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_22[] = {\n'+'\n'.join(','.join(map(str,body[i:i+32]))+',' for i in range(0,len(body),32))+'\n};\n')
t=(r/'src/art07.h').read_text()
t=re.sub(r'const Asset enemy_tiles_asset=.*?;\n','',t);t=re.sub(r'const u8 enemy_tile_map\[\]=.*?;\n','',t);t=re.sub(r'#define ENEMY_TILES \d+\n','',t)
t+='const Asset enemy_tiles_asset={22,0xA000};\nconst u8 enemy_tile_map[]={'+','.join(map(str,emap))+'};\n#define ENEMY_TILES %d\n'%len(tiles)
(r/'src/art07.h').write_text(t);print('tiles unicos',len(tiles),'bytes LZ',len(enc))
