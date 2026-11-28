#!/usr/bin/env python3
"""Icones 16x16 dos comandos do HUD (palette9, 4 bpp): uma faixa 256x16 que vai para a VRAM linhas 224..239 (padroes 896..959 da camada A).
Predios e unidades vem da arte de referencia do jogo (reduzida para 14x14 e quantizada para a paleta do HUD); parar, manter e melhorar sao desenhados aqui.
Saidas: src/banks/art_s71_b3.c (faixa LZ), src/icons_asset.h, reference/hud_icones.png (previa 6x).
Ordem: 0 parar, 1 manter, 2..8 casa quartel serraria ferraria estabulo torre oficina, 9..14 campones espadachim lanceiro arqueiro cavaleiro ariete, 15 melhorar."""
import re
import numpy as np
from PIL import Image
from pathlib import Path
r=Path(__file__).resolve().parents[1]
PAL5=[(0,0,0),(4,4,4),(4,9,4),(9,13,9),(4,4,9),(9,13,18),(9,4,4),(18,9,4),(13,9,4),(27,18,9),(22,18,9),(27,27,18),(9,9,9),(18,18,18),(27,27,27),(31,31,27)]
PAL=np.array([[v*255//31 for v in c] for c in PAL5],float)
PLATE,BORDER=12,14
def quant(rgb):
    d=((rgb[...,None,:]-PAL[None,None,1:,:])**2).sum(-1)   # nunca cor 0 (transparente)
    return d.argmin(-1)+1
def icon_from(img,box=None,bgtol=14):
    im=img.convert('RGB');a=np.array(im).astype(float)
    if box:a=a[box[1]:box[3],box[0]:box[2]]
    bg=a[0,0];alpha=(np.abs(a-bg).max(-1)>bgtol).astype(float)
    # tira ruido de borda (cinza dos paineis)
    ys,xs=np.where(alpha>0);y0,y1,x0,x1=ys.min(),ys.max()+1,xs.min(),xs.max()+1
    a=a[y0:y1,x0:x1];alpha=alpha[y0:y1,x0:x1]
    h,w=alpha.shape;s=max(h,w);pad=np.zeros((s,s,3));pa=np.zeros((s,s))
    oy,ox=(s-h)//2,(s-w)//2;pad[oy:oy+h,ox:ox+w]=a*alpha[...,None];pa[oy:oy+h,ox:ox+w]=alpha
    N=14;bs=s/N;out=np.zeros((N,N,3));oa=np.zeros((N,N))
    for j in range(N):
        for i in range(N):
            y_a,y_b=int(j*bs),max(int((j+1)*bs),int(j*bs)+1);x_a,x_b=int(i*bs),max(int((i+1)*bs),int(i*bs)+1)
            blk=pa[y_a:y_b,x_a:x_b];cnt=blk.sum();oa[j,i]=cnt/blk.size
            if cnt>0:out[j,i]=(pad[y_a:y_b,x_a:x_b].sum((0,1)))/cnt
    return out,oa
def plate(rgb=None,alpha=None,pix=None):
    p=np.full((16,16),PLATE,np.uint8)
    p[0,:]=p[15,:]=p[:,0]=p[:,15]=BORDER
    if pix is not None:
        for y,row in enumerate(pix):
            for x,ch in enumerate(row):
                if ch!='.':p[1+y,1+x]=int(ch,16)
        return p
    rgb=np.clip((rgb-30)*1.75+30,0,255)   # reduzidos ficam escuros: levanta o brilho e o contraste
    q=quant(rgb)
    for j in range(14):
        for i in range(14):
            if alpha[j,i]>=0.45:p[1+j,1+i]=q[j,i]
    return p
STOP=["....66666666....","...6777777776...","..677777777776..",".67777777777776.","6777FFFFFFFF7776","677FFFFFFFFFF776","677FFFFFFFFFF776","677FFFFFFFFFF776","677FFFFFFFFFF776","677FFFFFFFFFF776","677FFFFFFFFFF776","6777FFFFFFFF7776",".67777777777776.","..677777777776..","...6777777776...","....66666666...."]
STOP=[row[1:15] for row in STOP[1:15]]
HOLD=["..BBBBBBBBBB..","..B5555EE5555B.".replace('E','5')[:14],"..B5555555555B.","..B55EEEE5555B."]
def shield():
    rows=[]
    pat=["..CCCCCCCCCC..",".C5555555555C.",".C5555EE5555C.",".C555EEEE555C.",".C555EEEE555C.",".C5555EE5555C.",".C5555555555C.",".C5555555555C.","..C55555555C..","..C55555555C..","...C555555C...","....C5555C....",".....C55C.....","......CC......"]
    return [row.replace('C','B').replace('E','F') for row in pat]
def upgrade():
    return ["......BB......",".....BAAB.....","....BAAAAB....","...BAAAAAAB...","..BAAAAAAAAB..",".BBBBAAAABBBB.","....BAAAAB....","....BAAAAB....","....BAAAAB....","..DDDDDDDDDD..",".DCCCCCCCCCCD.","..DDDDDDDDDD..","..D8888888 D..".replace(' ',''),"..DDDDDDDDDD.."]
def main():
    ref=r/'reference/humanos_estilo'
    icons=[plate(pix=STOP if False else None) if False else None]*0
    out=[]
    out.append(plate(pix=[('.'+row.replace('F','E')+'.')[:14] if False else row for row in STOP]))
    out.append(plate(pix=shield()))
    for k in range(1,8):
        im=Image.open(ref/['predio_1_casa','predio_2_quartel','predio_3_serraria','predio_4_ferraria','predio_5_estabulo','predio_6_torre','predio_7_oficina_cerco'][k-1]).with_suffix('.png') if False else Image.open(ref/(['predio_1_casa','predio_2_quartel','predio_3_serraria','predio_4_ferraria','predio_5_estabulo','predio_6_torre','predio_7_oficina_cerco'][k-1]+'.png'))
        rgb,al=icon_from(im);out.append(plate(rgb,al))
    sheet=Image.open(ref/'humanos_referencia_unica.png')
    W=sheet.size[0]//6;H=sheet.size[1]//4
    for c in (0,1,3,2,4,5):   # na folha: campones, espadachim, arqueiro, lanceiro, cavaleiro, ariete; no jogo: lanceiro vem antes do arqueiro
        rgb,al=icon_from(sheet,(c*W,0,(c+1)*W,H));out.append(plate(rgb,al))
    out.append(plate(pix=upgrade()))
    strip=np.concatenate(out,axis=1)   # 16 x 256
    assert strip.shape==(16,256)
    # preview
    img=PAL[strip].astype(np.uint8);Image.fromarray(img).resize((256*6,16*6),Image.NEAREST).save(r/'reference/hud_icones.png')
    raw=bytearray()
    for y in range(16):
        for x in range(0,256,2):raw.append((int(strip[y,x])<<4)|int(strip[y,x+1]))
    src=(r/'tools/prepare09.py').read_text();ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns)
    blob=ns['lz'](bytes(raw));assert len(blob)<=8192
    (r/'src/banks/art_s71_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_71[] = {\n'+'\n'.join(','.join(map(str,blob[j:j+32]))+',' for j in range(0,len(blob),32))+'\n};\n')
    (r/'src/icons_asset.h').write_text('/* gerado por tools/hud_icons.py */\nconst Asset icons_asset={71,0xA000};\n')
    print('icones: faixa',len(raw),'B ->',len(blob),'B LZ')
if __name__=='__main__':main()
