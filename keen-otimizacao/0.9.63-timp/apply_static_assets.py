#!/usr/bin/env python3
"""0.9.63 (TIMP): conversões de dados estáticos que não passam pelos geradores do build.sh.
Uso: python3 apply_static_assets.py <pasta development062>   (idempotente)
 1. atlas.bin de scene/melon/bridge/tuberia: índice de cor 9 -> 1 (o 9 vira o segundo preto, usado pelo TIMP);
 2. paleta: entrada 9 = preto (src/scene_data.h);
 3. herói em bitmap (keen_msx_s27/s28_b1.c): máscara+cor -> uma imagem TIMP (0 transparente, 9 preto visível);
 4. cores dos sprites de hardware da cenoura (hw_carrot_data.h): cor 9 -> 1.
Os demais níveis passam pelo import_ega.planar (LUT 9->1) e são regenerados pelo build.sh.
"""
import re,sys
from pathlib import Path
r=Path(sys.argv[1])
for lv in ('scene','melon','bridge','tuberia'):
    f=r/'assets'/lv/'atlas.bin';b=bytearray(f.read_bytes())
    for i,v in enumerate(b):
        hi,lo=v>>4,v&15
        b[i]=((1 if hi==9 else hi)<<4)|(1 if lo==9 else lo)
    f.write_bytes(bytes(b))
p=r/'src/scene_data.h';s=p.read_text();s=s.replace("34,2,39,2,34,7,39,7,","34,2,0,0,34,7,39,7,");p.write_text(s)
for name in ('keen_msx_s27_b1.c','keen_msx_s28_b1.c'):
    p=r/'src'/name;t=p.read_text()
    m=re.search(r'(hero_bitmap\[14\]\[)(\d+)(\]=\{)(.*?)(\n\};)',t,re.S)
    if m.group(2)=='512':continue
    frames=re.findall(r'\{([0-9,]+)\}',m.group(4));assert len(frames)==14
    out=[]
    for fr in frames:
        b=list(map(int,fr.split(',')));assert len(b)==1024;res=[]
        for row in range(32):
            for mb,cb in zip(b[row*32:row*32+16],b[row*32+16:row*32+32]):
                v=0
                for sh in (4,0):
                    mk=(mb>>sh)&15;c=(cb>>sh)&15;assert mk in(0,15) and c!=9
                    v|=(0 if mk==15 else (c or 9))<<sh
                res.append(v)
        out.append('{'+','.join(map(str,res))+'}')
    p.write_text(t[:m.start()]+m.group(1)+'512'+m.group(3)+',\n'.join(out)+m.group(5)+t[m.end():])
p=r/'src/hw_carrot_data.h';t=p.read_text()
m=re.search(r'(carrot_colors\[[^=]*=\{)(.*?)(\};)',t,re.S)
nums=[int(x) for x in re.findall(r'\d+',m.group(2))]
p.write_text(t[:m.start(2)]+','.join(str((v&0xF0)|1 if v&15==9 else v) for v in nums)+t[m.end(2):])
print('static TIMP assets applied')
