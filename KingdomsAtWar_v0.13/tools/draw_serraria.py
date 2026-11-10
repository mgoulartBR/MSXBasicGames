#!/usr/bin/env python3
"""Serraria inimiga desenhada em codigo (64x64) na paleta do terreno. Saida: idx .npy (-1 = transparente)."""
import sys
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
sys.path.insert(0,str(Path(__file__).parent))
from convert_ramp import idx_of
C={n:idx_of(v) for n,v in dict(DK=(0x21,0x21,0x21),RD=(0x4A,0x21,0x21),BR=(0x6B,0x4A,0x21),RU=(0x94,0x4A,0x21),SD=(0xB5,0x94,0x4A),OR=(0xDE,0x94,0x4A),LT=(0xDE,0xDE,0x94),GR=(0x94,0x94,0x94),DG=(0x4A,0x4A,0x4A),WH=(0xFF,0xFF,0xDE)).items()}
im=Image.new('L',(64,64),0);d=ImageDraw.Draw(im)
def P(pts,c):d.polygon(pts,fill=C[c])
def R(x0,y0,x1,y1,c):d.rectangle([x0,y0,x1,y1],fill=C[c])
# chao
P([(32,45),(62,54),(32,63),(2,54)],'SD')
for y in range(46,63):
    for x in range(64):
        if im.getpixel((x,y))==C['SD'] and (x+y)%4==0:im.putpixel((x,y),C['OR'])
# interior escuro
R(10,36,54,52,'RD');R(10,36,54,38,'DK')
# toras (esquerda)
for cx,cy in((17,49),(24,49),(20.5,43)):
    d.ellipse([cx-4,cy-4,cx+4,cy+4],fill=C['BR']);d.ellipse([cx-2,cy-2,cx+2,cy+2],fill=C['SD'])
# serra circular (direita)
cx,cy=43,45
for a in range(0,360,30):
    x=cx+11*np.cos(np.radians(a));y=cy+11*np.sin(np.radians(a));d.ellipse([x-1.5,y-1.5,x+1.5,y+1.5],fill=C['LT'])
d.ellipse([cx-9,cy-9,cx+9,cy+9],fill=C['GR']);d.ellipse([cx-9,cy-9,cx+9,cy+9],outline=C['WH']);d.ellipse([cx-3,cy-3,cx+3,cy+3],fill=C['DG']);d.ellipse([cx-1,cy-1,cx+1,cy+1],fill=C['DK'])
# bancada
R(30,52,60,54,'BR')
# pilares
for x0 in(8,29,52):
    R(x0,34,x0+3,53,'BR');R(x0+3,34,x0+3,53,'RD')
# telhado: topo claro + borda grossa escura
P([(3,31),(32,13),(61,31),(32,38)],'SD')
P([(3,31),(32,13),(32,25)],'LT')
P([(3,31),(32,38),(32,41),(3,34)],'BR');P([(32,38),(61,31),(61,34),(32,41)],'RD')
d.line([(3,31),(32,13),(61,31)],fill=C['BR'],width=1)
# bandeira
R(31,3,32,14,'BR');P([(33,3),(40,3),(38,6),(40,9),(33,9)],'RU')
a=np.array(im).astype(int);a=np.where(a==0,-1,a)
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);np.save(out/'serraria_idx.npy',a)
