#!/usr/bin/env python3
"""Substitui as poses do soldado inimigo (tipo 6, chaves 144..159) por uma folha convertida (convert_sheet.py).
uso: integrate_enemy.py prefixo   (le prefixo_idx.npy e prefixo_pal.npy)
Reescreve o fim do banco 9 e o banco 10 (dados LZ), as referencias, os limites e as celulas dessas poses em art07.h e
a paleta propria do inimigo (grupo 1 dos sprites): enemy_palette[], 15 cores 0..31 + transparente."""
import re,sys
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1];pre=sys.argv[1]
idx=np.load(pre+'_idx.npy');pal=np.load(pre+'_pal.npy')
src=(r/'tools/prepare09.py').read_text()
ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns);lz=ns['lz']
def pack(a):return bytes((a[:,::2]*16+a[:,1::2]).flatten())
t=(r/'src/art07.h').read_text()
refs=[(int(a),int(b,0)) for a,b in re.findall(r'\{(\d+),(0x[0-9A-F]+|\d+)\}',re.search(r'poses\[\]\s*=\s*\{(.*?)\};',t,re.S)[1])]
bounds=list(map(int,re.search(r'pose_bounds\[\]=\{([^}]+)',t)[1].split(',')))
cells=list(map(int,re.search(r'pose_cells\[\]=\{([^}]+)',t)[1].split(',')))
b9=[int(x) for x in re.findall(r'\d+',(r/'src/banks/art_s9_b3.c').read_text().split('{',1)[1])]
start=refs[144][1]-0xA000;assert refs[144][0]==9
bank9=bytearray(b9[:start]);bank10=bytearray();cur=bank9;bank=9
newrefs=[]
for k in range(144,168):
    f=k-144
    if f>=16:newrefs.append((0,0));continue
    cell=idx[(f//4)*32:(f//4)*32+32,(f%4)*32:(f%4)*32+32]
    a=np.where(cell>=0,cell+1,0).astype(np.uint8)
    raw=b''.join(pack(a[y*16:y*16+16,x*16:x*16+16]) for y in range(2) for x in range(2))
    enc=lz(raw)
    if bank==9 and len(bank9)+len(enc)>8192:cur=bank10;bank=10
    newrefs.append((bank,0xA000+len(cur)));cur.extend(enc)
    yy,xx=np.nonzero(a);l,tt,w,h=int(xx.min()),int(yy.min()),int(xx.max()-xx.min()+1),int(yy.max()-yy.min()+1)
    bounds[k*4:k*4+4]=[l,tt,w,h]
    cx0=l>>4;cy0=tt>>4;cols=((l+w-1)>>4)-cx0+1;rows=((tt+h-1)>>4)-cy0+1
    cells[k*2:k*2+2]=[cy0*16+cx0,cols*16+rows]
assert len(bank10)<=8192
refs[144:168]=newrefs
def cfile(name,n,data):
    (r/'src/banks'/f'art_s{n}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%n+'\n'.join(','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32))+'\n};\n')
cfile('b9',9,bank9);cfile('b10',10,bank10 if bank10 else bytearray([0]))
refs_s='const Asset poses[]={'+','.join('{%d,0x%04X}'%x if x[0] else '{0,0}' for x in refs)+'};'
t=re.sub(r'const Asset poses\[\]=\{.*?\};',refs_s,t,count=1,flags=re.S)
t=re.sub(r'pose_bounds\[\]=\{[^}]+\}','pose_bounds[]={'+','.join(map(str,bounds))+'}',t,count=1)
t=re.sub(r'pose_cells\[\]=\{[^}]+\}','pose_cells[]={'+','.join(map(str,cells))+'}',t,count=1)
ep=[0,0,0]+[min(31,round(int(v)*31/255)) for c in pal for v in c]
line='const u8 enemy_palette[]={%s};\n'%','.join(map(str,ep))
t=re.sub(r'const u8 enemy_palette\[\]=\{[^}]*\};\n','',t)
i=t.index('const u8 pose_cells[]');j=t.index('\n',i);t=t[:j+1]+line+t[j+1:]
(r/'src/art07.h').write_text(t)
print('banco9',len(bank9),'banco10',len(bank10),'novas refs',newrefs[:3])
