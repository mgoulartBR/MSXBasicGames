#!/usr/bin/env python3
"""Integra as poses da facção inimiga (convert_units.py) ao jogo: tipos de pose 6..11 (sword, peasant, spear, archer, knight, ram).
Reindexa as poses por chave contigua (pose_base), grava os dados LZ nos bancos 16.., atualiza art07.h (poses, pose_bounds,
pose_cells, key_slot, pose_base, pose_count, skin_map, kind_sizes, enemy_palette).
uso: integrate_units.py dir_com_idx   (a mesma saida de convert_units.py)"""
import re,sys
from pathlib import Path
import numpy as np
r=Path(__file__).resolve().parents[1];d=Path(sys.argv[1])
src=(r/'tools/prepare09.py').read_text();ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns);lz=ns['lz']
def pack(a):return bytes((a[:,::2]*16+a[:,1::2]).flatten())
t=(r/'src/art07.h').read_text()
def arr(name):return list(map(int,re.search(name+r'\[\]=\{([^}]+)',t)[1].split(',')))
refs=[(int(a),int(b,0)) for a,b in re.findall(r'\{(\d+),(0x[0-9A-F]+|\d+)\}',re.search(r'poses\[\]\s*=\s*\{(.*?)\};',t,re.S)[1])]
bounds=arr('pose_bounds');cells=arr('pose_cells')
COUNT=[24,16,16,16,16,16,16,24,16,16,16,16];SIZES=[32,32,48,32,48,48,32,32,48,32,48,48]
BASE=[sum(COUNT[:k]) for k in range(12)];TOTAL=sum(COUNT)
if len(refs)==168:
    old=[(0,0),(24,24),(48,16),(72,16),(96,16),(120,16)]   # chaves antigas (inicio, quantidade) dos tipos humanos 0..5
    nrefs=[(0,0)]*TOTAL;nb=[0]*(TOTAL*4);nc=[0]*(TOTAL*2)
    for k,(o,_) in zip(range(6),[(0,0),(24,0),(48,0),(72,0),(96,0),(120,0)]):
        for f in range(COUNT[k]):
            ok=k*24+f;nk=BASE[k]+f;nrefs[nk]=refs[ok];nb[nk*4:nk*4+4]=bounds[ok*4:ok*4+4];nc[nk*2:nk*2+2]=cells[ok*2:ok*2+2]
    # dados antigos do tipo inimigo (banco 9 a partir de refs[144]) sao descartados
    cut=refs[144][1]-0xA000;b9=[int(x) for x in re.findall(r'\d+',(r/'src/banks/art_s9_b3.c').read_text().split('{',1)[1])][:cut]
    def cfile(n,data):(r/'src/banks'/f'art_s{n}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%n+'\n'.join(','.join(map(str,data[i:i+32]))+',' for i in range(0,len(data),32))+'\n};\n')
    cfile(9,b9);cfile(10,[0])
else:
    nrefs=refs[:];nb=bounds[:];nc=cells[:]
names=['sword','peasant','spear','archer','knight','ram'];skin=[6,7,8,9,10,11]
bank=16;chunk=bytearray();banks={}
def flush():
    global chunk,bank
    if chunk:banks[bank]=bytes(chunk);bank+=1;chunk=bytearray()
for nm,kind in zip(names,skin):
    # a folha desta unidade: ordem de arquivo sword, peasant, archer, spear, knight, ram
    idx=np.load(d/f'{nm}_idx.npy');n=SIZES[kind]//16;sz=SIZES[kind]
    for f in range(COUNT[kind]):
        if kind==7:   # peasant: linhas 1-4 caminhada (d*4+fase), linha 5 trabalho A (16+d), linha 6 trabalho B (20+d)
            row,col=(f//4,f%4) if f<16 else ((4,f-16) if f<20 else (5,f-20))
        else:row,col=f//4,f%4
        cell=idx[row*sz:(row+1)*sz,col*sz:(col+1)*sz];a=np.where(cell>=0,cell+1,0).astype(np.uint8)
        raw=b''.join(pack(a[y*16:y*16+16,x*16:x*16+16]) for y in range(n) for x in range(n));enc=lz(raw)
        if len(chunk)+len(enc)>8192:flush()
        key=BASE[kind]+f;nrefs[key]=(bank,0xA000+len(chunk));chunk.extend(enc)
        yy,xx=np.nonzero(a);l,tt,w,h=int(xx.min()),int(yy.min()),int(xx.max()-xx.min()+1),int(yy.max()-yy.min()+1)
        nb[key*4:key*4+4]=[l,tt,w,h];cx0=l>>4;cy0=tt>>4;cols=((l+w-1)>>4)-cx0+1;rows=((tt+h-1)>>4)-cy0+1;nc[key*2:key*2+2]=[cy0*16+cx0,cols*16+rows]
flush()
for b,data in banks.items():
    (r/'src/banks'/f'art_s{b}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%b+'\n'.join(','.join(str(v) for v in data[i:i+32])+',' for i in range(0,len(data),32))+'\n};\n')
slot={};base32=0;base48=0;key_slot=[0]*TOTAL
for k in range(12):
    if SIZES[k]==32:slot[k]=base32;base32+=COUNT[k]
    else:slot[k]=base48;base48+=COUNT[k]
    for f in range(COUNT[k]):key_slot[BASE[k]+f]=slot[k]+f
print('poses 32:',base32,'poses 48:',base48,'bancos',sorted(banks),'bytes',{b:len(v) for b,v in banks.items()})
pal=np.load(d/'palette.npy');ep=[0,0,0]+[min(31,round(int(v)*31/255)) for c in pal for v in c]
def sub(pattern,repl):
    global t
    t=re.sub(pattern,lambda m:repl,t,count=1,flags=re.S)
sub(r'const Asset poses\[\]=\{.*?\};','const Asset poses[]={'+','.join('{%d,0x%04X}'%x if x[0] else '{0,0}' for x in nrefs)+'};')
sub(r'pose_bounds\[\]=\{[^}]+\}','pose_bounds[]={'+','.join(map(str,nb))+'}')
sub(r'pose_cells\[\]=\{[^}]+\}','pose_cells[]={'+','.join(map(str,nc))+'}')
sub(r'kind_sizes\[\]=\{[^}]+\}','kind_sizes[]={'+','.join(map(str,SIZES))+'}')
t=re.sub(r'const u8 (enemy_palette|key_slot|pose_base|pose_count|skin_map)\[\]=\{[^}]*\};\n','',t)
extra='const u8 enemy_palette[]={%s};\nconst u8 key_slot[]={%s};\nconst u8 pose_base[]={%s};\nconst u8 pose_count[]={%s};\nconst u8 skin_map[]={7,6,8,9,10,11,6};\n'%(','.join(map(str,ep)),','.join(map(str,key_slot)),','.join(map(str,BASE)),','.join(map(str,COUNT)))
i=t.index('const u8 pose_cells[]');j=t.index('\n',i);t=t[:j+1]+extra+t[j+1:]
(r/'src/art07.h').write_text(t)
