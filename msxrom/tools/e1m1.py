"""Conversor do E1M1 (a partir do doom1.wad DO USUARIO) para o formato da ROM. Roda localmente; nada do WAD nem dos dados
convertidos vai para o repositorio (private/ esta no .gitignore).

Marco 1: pedacos de parede com alinhamento de textura do Doom (xoffset/yoffset, peg superior/inferior), atlas de texturas,
         vista de verificacao pelo modelo de referencia do Geo3D."""
import sys,os,math,collections,struct
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.join(HERE,'..')
sys.path.insert(0,HERE)
from wad import Wad

SKY='F_SKY1'

# ----------------------------------------------------------------------------------------------- pedacos de parede
def wall_pieces(w,m):
    """Um 'pedaco' = trecho de parede visto de um lado: A->B com o observador a DIREITA de A->B (convencao do Doom).
    Campos: A,B (coordenadas do mapa), ybot,ytop, tex, tw,th (tamanho da textura), xoff, texmid (altura do mundo que corresponde a linha 0
    da textura, ja com yoffset), kind, sector, line."""
    pieces=[]; vx=m.vertexes
    def tinfo(name):
        tw,th,_=w.texture_info(name); return tw,th
    for li,l in enumerate(m.linedefs):
        for side in ('right','left'):
            si=l[side]
            if si<0: continue
            sd=m.sidedefs[si]; fs=m.sectors[sd['sector']]
            A,B=(vx[l['v1']],vx[l['v2']]) if side=='right' else (vx[l['v2']],vx[l['v1']])
            osi=l['left' if side=='right' else 'right']
            fl=l['flags']
            def add(kind,ybot,ytop,tex,texmid):
                if ytop<=ybot or tex in ('-',''): return
                tw,th=tinfo(tex)
                pieces.append(dict(A=A,B=B,ybot=ybot,ytop=ytop,tex=tex,tw=tw,th=th,xoff=sd['xoff'],texmid=texmid+sd['yoff'],
                                   kind=kind,sector=sd['sector'],line=li,side=side))
            if osi<0:                                                   # linha de um lado: textura do meio
                if sd['mid'] in ('-',''): continue
                tw,th=tinfo(sd['mid'])
                texmid=(fs['floor']+th) if fl&16 else fs['ceil']           # ML_DONTPEGBOTTOM
                add('mid',fs['floor'],fs['ceil'],sd['mid'],texmid)
            else:
                bs=m.sectors[m.sidedefs[osi]['sector']]
                # superior
                if fs['ceil']>bs['ceil'] and not (fs['ctex']==SKY and bs['ctex']==SKY) and sd['upper'] not in ('-',''):
                    tw,th=tinfo(sd['upper'])
                    texmid=fs['ceil'] if fl&8 else bs['ceil']+th                # ML_DONTPEGTOP
                    add('upper',bs['ceil'],fs['ceil'],sd['upper'],texmid)
                # inferior
                if fs['floor']<bs['floor'] and sd['lower'] not in ('-',''):
                    texmid=fs['ceil'] if fl&16 else bs['floor']                 # ML_DONTPEGBOTTOM
                    add('lower',fs['floor'],bs['floor'],sd['lower'],texmid)
    return pieces

def split_piece(p):
    """Divide o pedaco nas repeticoes da textura. Devolve subquads: (d0,d1,hb,ht,u0,u1,r0,r1) onde d = distancia ao longo de A->B,
    u em unidades dentro de [0,tw], r = linha em unidades dentro de [0,th] (r0 no topo)."""
    (ax,ay),(bx,by)=p['A'],p['B']; L=math.hypot(bx-ax,by-ay); tw,th=p['tw'],p['th']
    xs=[0.0]; d=0.0
    # cortes horizontais nos multiplos de tw
    start=(p['xoff'])%tw; first=tw-start if start>0 else tw
    d=first
    while d<L-1e-6: xs.append(d); d+=tw
    xs.append(L)
    hs=[p['ybot']]; texmid=p['texmid']
    # cortes verticais: linhas r=texmid-h multiplos de th
    r_top=texmid-p['ytop']; r_bot=texmid-p['ybot']
    k=math.floor(r_top/th)+1
    cuts=[]
    while k*th<r_bot-1e-6: cuts.append(texmid-k*th); k+=1                # alturas do mundo nos limites
    hs=[p['ybot']]+sorted(cuts)+[p['ytop']]
    hs=sorted(set(hs))
    out=[]
    for i in range(len(xs)-1):
        d0,d1=xs[i],xs[i+1]; u0=(p['xoff']+d0)%tw; u1=u0+(d1-d0)
        if u1>tw+1e-6: u1=tw
        for j in range(len(hs)-1):
            hb,ht=hs[j],hs[j+1]
            r0=(texmid-ht)%th; r1=r0+(ht-hb)
            if r1>th+1e-6: r1=th
            out.append((d0,d1,hb,ht,u0,u1,r0,r1))
    return out

# ----------------------------------------------------------------------------------------------- atlas de texturas
ATLAS_W=256; BAND=64; NBANDS=4          # 4 faixas de 64 linhas = 256 linhas (VRAM 512..767)
def tile_dims(tw,th,s): return max(1,round(tw*s)),max(1,round(th*s))

def shelf_pack(items):
    """items: [(nome,w,h)] com h<=64. Prateleiras = faixas de 64 linhas. Devolve {nome:(x,y)} ou None se nao couber."""
    shelves=[[0]*1 for _ in range(NBANDS)]; pos={}
    for nm,tw,th in sorted(items,key=lambda e:-e[1]):
        for b in range(NBANDS):
            if shelves[b][0]+tw<=ATLAS_W:
                pos[nm]=(shelves[b][0],b*BAND); shelves[b][0]+=tw; break
        else: return None
    return pos

def choose_scales(w,pieces,budget_px=56000):
    """Escala 0,25 texel/unidade para tudo; sobe para 0,5 as texturas de maior area na tela enquanto couber."""
    info={}
    for p in pieces: info.setdefault(p['tex'],[p['tw'],p['th'],0.0]); info[p['tex']][2]+=math.hypot(p['B'][0]-p['A'][0],p['B'][1]-p['A'][1])*(p['ytop']-p['ybot'])
    sc={k:0.25 for k in info}
    def items(): return [(k,)+tile_dims(info[k][0],info[k][1],sc[k]) for k in info]
    order=sorted(info,key=lambda k:-info[k][2])
    for k in order:
        old=sc[k]; sc[k]=0.5
        its=items()
        if sum(a*b for _,a,b in its)>budget_px or any(th>BAND for _,_,th in its) or shelf_pack(its) is None: sc[k]=old
    return sc,info

def build_atlas(w,sc,info):
    its=[(k,)+tile_dims(info[k][0],info[k][1],sc[k]) for k in info]; pos=shelf_pack(its); assert pos
    pal=w.playpal(0); rgb=np.zeros((NBANDS*BAND,ATLAS_W,3),np.uint8); alpha=np.zeros((NBANDS*BAND,ATLAS_W),bool); tiles={}
    for k,tw,th in its:
        idx=w.texture(k); full=np.where(idx[...,None]>=0,pal[np.clip(idx,0,255)],0).astype(np.float32)
        a=(idx>=0).astype(np.float32)
        H,W=idx.shape
        # reducao por media de caixa (area)
        img=Image.fromarray(full.astype(np.uint8)).resize((tw,th),Image.BOX)
        am=Image.fromarray((a*255).astype(np.uint8)).resize((tw,th),Image.BOX)
        x,y=pos[k]; rgb[y:y+th,x:x+tw]=np.array(img); alpha[y:y+th,x:x+tw]=np.array(am)>127
        tiles[k]=dict(x=x,y=y,w=tw,h=th,sx=tw/info[k][0],sy=th/info[k][1],tw=info[k][0],th=info[k][1])
    return rgb,alpha,tiles

if __name__=='__main__':
    wadp=sys.argv[1]; w=Wad(wadp); m=w.load_map('E1M1')
    pcs=wall_pieces(w,m); print('pedacos',len(pcs),collections.Counter(p['kind'] for p in pcs))
    sub=sum(len(split_piece(p)) for p in pcs); print('subquads apos repeticao de textura',sub)
    sc,info=choose_scales(w,pcs); print('texturas a 0,5:',sum(1 for v in sc.values() if v==0.5),'de',len(sc))
    rgb,alpha,tiles=build_atlas(w,sc,info)
    os.makedirs(os.path.join(ROOT,'private'),exist_ok=True)
    Image.fromarray(rgb).resize((ATLAS_W*3,NBANDS*BAND*3),Image.NEAREST).save(os.path.join(ROOT,'private','atlas_rgb.png'))
    print('atlas ok',rgb.shape)
