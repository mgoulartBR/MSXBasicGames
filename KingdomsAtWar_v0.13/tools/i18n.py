#!/usr/bin/env python3
"""Textos em 4 idiomas + fonte 8x8 estendida.
- STRINGS: id -> (pt, en, es, ja). Latim em MAIUSCULAS (a fonte base e 5x7 maiuscula); acentos e kana viram glifos extras
  alocados dinamicamente nos codigos 96..255 (padroes 736..895 da camada A, linhas 184..223).
- Acentos: letras de 5 linhas (linhas 2..6) com o acento nas linhas 0..1 (cedilha na linha 7), desenhadas aqui a mao.
- Kana: renderizados pelo IPAGothic em 9 px e limpos; sons vozeados = kana base + marca (゛ ゜) em celula propria, como
  no kana de meia largura do MSX.
Saidas: src/text.h (ids, TEXT_BANK), src/text_asset.h (font_ext_asset, so main.c), src/banks/art_s51_b3.c (tabela de textos), art_s52_b3.c (fonte LZ),
        reference/fonte/especime.png (previa)."""
import re,sys,unicodedata
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
r=Path(__file__).resolve().parents[1]
LANGS=4
STRINGS=[
 ('PRESS',('PRESSIONE ENTER','PRESS ENTER','PULSE ENTER','エンターキーをおしてください')),
 ('NEW_GAME',('NOVO JOGO','NEW GAME','NUEVO JUEGO','あたらしいゲーム')),
 ('LANGUAGE',('IDIOMA','LANGUAGE','IDIOMA','げんご')),
 ('TITLE',('REINOS','KINGDOMS','REINOS','レイノス')),
 ('SUBTITLE',('OS INSEPULTOS','THE UNBURIED','LOS INSEPULTOS','ほうむられざるもの')),
 ('LANG_PT',('PORTUGUÊS',)*4),
 ('LANG_EN',('ENGLISH',)*4),
 ('LANG_ES',('ESPAÑOL',)*4),
 ('LANG_JA',('ニホンゴ',)*4),
]
# ---- fonte base (5x7 maiuscula, ver art/reference_v68_font.bin)
b=np.frombuffer(open(r/'art/reference_v68_font.bin','rb').read(),dtype=np.uint8)
STRIP=np.zeros((16,256),np.uint8);STRIP[:,0::2]=(b>>4).reshape(16,128);STRIP[:,1::2]=(b&15).reshape(16,128)
def base(c):
    k=ord(c)-32;y=(k//32)*8;x=(k%32)*8;return STRIP[y:y+8,x:x+8]
def bits(rows):  # lista de strings '#'/'.' de 8 colunas -> matriz 0/1
    return np.array([[1 if ch=='#' else 0 for ch in row.ljust(8,'.')[:8]] for row in rows],np.uint8)
# letras de 5 linhas (colunas 1..5) para os acentuados
L5={'A':['.###.','#...#','#####','#...#','#...#'],'E':['#####','#....','###..','#....','#####'],'I':['.###.','..#..','..#..','..#..','.###.'],
 'O':['.###.','#...#','#...#','#...#','.###.'],'U':['#...#','#...#','#...#','#...#','.###.'],'C':['.###.','#...#','#....','#...#','.###.'],
 'N':['#...#','##..#','#.#.#','#..##','#...#']}
ACC={'´':['...#.','..#..'],'`':['.#...','..#..'],'^':['..#..','.#.#.'],'~':['.##.#','#..#.'],'¨':['.....','.#.#.']}
def glyph_acc(letter,acc):
    rows=['.'+x for x in ACC[acc]]+['.'+x for x in L5[letter]]
    rows=['.'+x[1:] if False else x for x in rows]
    m=bits(['.'+x if len(x)==5 else x for x in [r_[1:] if r_.startswith('.') and len(r_)==6 else r_ for r_ in rows]]) if False else None
    out=np.zeros((8,8),np.uint8)
    for i,row in enumerate(ACC[acc]):
        for j,ch in enumerate(row):out[i,1+j]=ch=='#'
    for i,row in enumerate(L5[letter]):
        for j,ch in enumerate(row):out[2+i,1+j]=ch=='#'
    return out
def glyph_cedilla():
    out=np.zeros((8,8),np.uint8)
    for i,row in enumerate(L5['C']):
        for j,ch in enumerate(row):out[2+i,1+j]=ch=='#'
    out[7,3]=1;out[6,3]=out[6,3];return out
SPECIAL={}
for ch,(l,a) in {'Á':('A','´'),'À':('A','`'),'Â':('A','^'),'Ã':('A','~'),'É':('E','´'),'È':('E','`'),'Ê':('E','^'),'Í':('I','´'),
  'Ó':('O','´'),'Ô':('O','^'),'Õ':('O','~'),'Ú':('U','´'),'Ü':('U','¨'),'Ñ':('N','~')}.items():SPECIAL[ch]=glyph_acc(l,a)
SPECIAL['Ç']=glyph_cedilla()
SPECIAL['¿']=bits(['........','...#....','........','...#....','..#.....','.#...#..','..###...','........'])
SPECIAL['¡']=bits(['........','...#....','........','...#....','...#....','...#....','...#....','........'])
SPECIAL['゛']=bits(['..#.#...','.#.#....','........','........','........','........','........','........'])
SPECIAL['゜']=bits(['..##....','.#..#...','..##....','........','........','........','........','........'])
SPECIAL['。']=bits(['........','........','........','........','........','..##....','..##....','........'])
SPECIAL['、']=bits(['........','........','........','........','..#.....','..#.....','.#......','........'])
SPECIAL['「']=bits(['..####..','..#.....','..#.....','..#.....','........','........','........','........'])
SPECIAL['」']=bits(['........','........','........','.....#..','.....#..','.....#..','..####..','........'])
IPAG='/usr/share/fonts/opentype/ipafont-gothic/ipag.ttf'
def kana(ch):
    best=None
    for size in (9,8):
        f=ImageFont.truetype(IPAG,size);im=Image.new('L',(16,16),0);d=ImageDraw.Draw(im);d.fontmode='1';d.text((2,1),ch,font=f,fill=255)
        a=(np.array(im)>128).astype(np.uint8);ys,xs=np.nonzero(a)
        if not len(ys):continue
        a=a[ys.min():ys.max()+1,xs.min():xs.max()+1]
        if a.shape[0]<=8 and a.shape[1]<=8:best=a;break
        best=best if best is not None else a
    h,w=best.shape;out=np.zeros((8,8),np.uint8);h=min(h,8);w=min(w,8)
    y0=(8-h)//2 if h<7 else 0;x0=(8-w)//2
    out[y0:y0+h,x0:x0+w]=best[:h,:w];return out
def expand_text(s):
    """NFD para o kana: vozeado vira base + marca."""
    out=[]
    for ch in s:
        d=unicodedata.normalize('NFD',ch)
        if len(d)==2 and d[1] in '゙゚':out+= [d[0],'゛' if d[1]=='゙' else '゜']
        else:out.append(ch)
    return out
def build():
    used=[];texts=[]
    for sid,tr_ in STRINGS:
        row=[]
        for lang,s in enumerate(tr_):
            s=s.upper() if lang<3 else s
            chars=expand_text(s);row.append(chars)
            for ch in chars:
                if ord(ch)>=96 or ch in '`':
                    if ch not in used:used.append(ch)
        texts.append((sid,row))
    if len(used)>160:sys.exit('glifos demais: %d > 160'%len(used))
    code={ch:96+i for i,ch in enumerate(used)}
    ext=np.ones((40,256),np.uint8)  # fundo = cor 1 como a fonte base
    for ch,c in code.items():
        g=SPECIAL[ch] if ch in SPECIAL else kana(ch);k=c-96;y=(k//32)*8;x=(k%32)*8
        ext[y:y+8,x:x+8]=np.where(g>0,15,1)
    return texts,code,ext
def lz_pack(a):
    src=(r/'tools/prepare09.py').read_text();ns={};exec(re.search(r'def lz\(a\):.*?return bytes\(out\)',src,re.S)[0],ns)
    return ns['lz'](bytes((a[:,::2]*16+a[:,1::2]).flatten()))
def main():
    texts,code,ext=build()
    # tabela: ids x idiomas -> ponteiro; strings terminadas em 0
    n=len(texts);off=n*LANGS*2;blob=bytearray();ptrs=[]
    for sid,row in texts:
        for chars in row:
            ptrs.append(0xA000+off+len(blob));blob+=bytes(code.get(ch,ord(ch)) for ch in chars)+b'\0'
    tab=b''.join(p.to_bytes(2,'little') for p in ptrs)
    # ptrs sao enderecos absolutos na ordem [id][lang]; leitura: (id*LANGS+lang)*2
    body=tab+bytes(blob);assert len(body)<=8192
    def cfile(num,data):
        (r/'src/banks'/f'art_s{num}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%num+'\n'.join(','.join(map(str,data[j:j+32]))+',' for j in range(0,len(data),32))+'\n};\n')
    cfile(51,body);fnt=lz_pack(ext);assert len(fnt)<=8192;cfile(52,fnt)
    h='/* gerado por tools/i18n.py */\n#define TEXT_BANK 51\n#define TXT_COUNT %d\n#define LANGS 4\n'%n+''.join('#define TXT_%s %d\n'%(sid,i) for i,(sid,_) in enumerate(texts))
    (r/'src/text.h').write_text(h)
    (r/'src/text_asset.h').write_text('/* gerado por tools/i18n.py */\nconst Asset font_ext_asset={52,0xA000};\n')
    out=r/'reference/fonte';out.mkdir(exist_ok=True)
    pal=np.array([(0,0,0),(0x21,0x21,0x21)]+[(0,0,0)]*13+[(0xFF,0xFF,0xDE)],np.uint8)
    Image.fromarray(pal[ext]).resize((256*4,40*4),Image.NEAREST).save(out/'especime_extra.png')
    print('glifos extras',len(code),'de 160; texto',len(body),'B; fonte LZ',len(fnt),'B')
if __name__=='__main__':main()
