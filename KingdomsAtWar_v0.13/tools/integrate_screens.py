#!/usr/bin/env python3
"""Telas de 256x160 (camada A: patterns 0..639 em identidade com a tabela de nomes) para bancos de ROM.
Le reference/telas/<nome>_idx.npy (192x256, indices 0..15; so as 160 primeiras linhas entram), corta em 4 blocos de 40 linhas,
comprime cada bloco (LZ do jogo) e empacota em bancos a partir do segmento 32 (maximo 8192 B por banco).
Grava src/banks/art_s<N>_b3.c e src/art08.h (Asset screen_chunks[8*4]) e src/screens.h (ids SCR_*)."""
import re
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1]
src=(r/'tools/prepare09.py').read_text();ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns);lz=ns['lz']
def pack(a):return bytes((a[:,::2]*16+a[:,1::2]).flatten())
NAMES=['Titlescreen','VictoryHuman','DefeatHuman','EndCampainHuman','Menu_Briefing','VictoryInsepultos','DefeatInsepultos','EndCampainInsepultos']
IDS=['TITLE','VICTORY_HUMAN','DEFEAT_HUMAN','END_HUMAN','MENU','VICTORY_UNDEAD','DEFEAT_UNDEAD','END_UNDEAD']
FIRST=32;banks=[bytearray()];refs=[]
for n in NAMES:
    a=np.load(r/f'reference/telas/{n}_idx.npy')[:160]
    for k in range(4):
        blob=lz(pack(a[k*40:(k+1)*40]));assert len(blob)<=8192
        if len(banks[-1])+len(blob)>8192:banks.append(bytearray())
        refs.append((FIRST+len(banks)-1,0xA000+len(banks[-1])));banks[-1]+=blob
for i,b in enumerate(banks):
    body=bytes(b)
    (r/'src/banks'/f'art_s{FIRST+i}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%(FIRST+i)+'\n'.join(','.join(map(str,body[j:j+32]))+',' for j in range(0,len(body),32))+'\n};\n')
(r/'src/screens.h').write_text('/* gerado por tools/integrate_screens.py */\n'+''.join('#define SCR_%s %d\n'%(n,i) for i,n in enumerate(IDS)))
h='/* gerado por tools/integrate_screens.py */\n'
h+='const Asset screen_chunks[]={'+','.join('{%d,0x%X}'%t for t in refs)+'};\n'
(r/'src/art08.h').write_text(h)
print('bancos',FIRST,'..',FIRST+len(banks)-1,'bytes',[len(b) for b in banks])
