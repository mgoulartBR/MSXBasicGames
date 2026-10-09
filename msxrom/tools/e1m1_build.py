"""Gera os dados PRIVADOS da ROM do E1M1 a partir do doom1.wad do usuario: private/e1m1_assets.h e private/e1m1_banks.bin.
NADA daqui vai para o repositorio. uso: e1m1_build.py doom1.wad [skill 1..5]"""
import sys,os,math,struct
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.join(HERE,'..'); sys.path.insert(0,HERE)
import e1m1,e1m1_map,e1m1_pvs as P
from make_assets import median_cut,WEIGHT
A=os.path.join(ROOT,'assets'); PRIV=os.path.join(ROOT,'private')
SKILL=int(sys.argv[2]) if len(sys.argv)>2 else 3
SKILL_FLAG={1:1,2:1,3:2,4:4,5:4}[SKILL]

# ---------------------------------------------------------------------------------------------- imagens
def to_rgba(w,idx,pal):
    a=np.zeros(idx.shape+(4,),np.uint8); m=idx>=0; a[m,:3]=pal[idx[m]]; a[m,3]=255; return a
def scaled(rgba,s):
    h,wd=rgba.shape[:2]; nw,nh=max(1,round(wd*s)),max(1,round(h*s))
    pm=rgba.astype(np.float32); pm[...,:3]*=pm[...,3:4]/255
    im=Image.fromarray(pm.astype(np.uint8),'RGBA')
    parts=[np.array(Image.fromarray(pm[...,k].astype(np.uint8)).resize((nw,nh),Image.BOX),np.float32) for k in range(4)]
    a=parts[3]; out=np.zeros((nh,nw,4),np.uint8); m=a>0
    for k in range(3): out[...,k][m]=np.clip(parts[k][m]/a[m]*255,0,255)
    out[...,3]=np.where(a>=110,255,0); return out
def png(name): return np.array(Image.open(os.path.join(A,name+'.png')).convert('RGBA'),np.uint8)

# ---------------------------------------------------------------------------------------------- sprites / hud
SPRITES=['POSSA1','POSSB1','POSSE1','POSSG1','POSSI0','POSSL0',
         'SPOSA1','SPOSB1','SPOSE1','SPOSG1','SPOSI0','SPOSL0',
         'TROOA1','TROOB1','TROOF1','TROOH1','TROOJ0','TROOM0',
         'STIMA0','MEDIA0','BON1A0','BON2A0','ARM1A0','CLIPA0','AMMOA0','SBOXA0','SHELA0','SHOTA0','BAR1A0','COLUA0','ELECA0','CBRAA0','BEXPA0','ARM2A0','SHEL_BOX']
SPRITES=[s for s in SPRITES if s!='SHEL_BOX']+['SBOXA0']
SPRITES=list(dict.fromkeys(SPRITES))
SLOT={n:i for i,n in enumerate(SPRITES)}
CW,CH=24,32
def build_sprite_cells(w,pal):
    cells=np.zeros((4*CH,10*CW,4),np.uint8); info=[]
    for i,n in enumerate(SPRITES):
        idx,lo,to=w.picture(n); rgba=to_rgba(w,idx,pal); h,wd=idx.shape
        s=0.5 if n[:4] in ('POSS','SPOS','TROO','BEXP') else 1.0
        s=min(s,(CH)/h,(CW-2)/wd) if True else s
        if n[:4] in ('POSS','SPOS','TROO','BEXP'): s=min(0.5,s)
        sc=scaled(rgba,s); sh,sw=sc.shape[:2]
        x0=int(round(CW/2-lo*s)); x0=min(max(0,x0),CW-sw); y0=CH-sh
        cx,cy=(i%10)*CW,(i//10)*CH; cells[cy+y0:cy+y0+sh,cx+x0:cx+x0+sw]=sc
        info.append((1.0/s))                                          # unidades do mundo por texel
    return cells,info

GUN=[('PISGA0',0.65),('PISGB0',0.65),('PISGC0',0.65),('PISGD0',0.65),('PISFA0',0.65)]
FACES=['STFST00','STFST10','STFST20','STFST30','STFST40','STFDEAD0']
DIGITS=['STTNUM%d'%i for i in range(10)]+['STTPRCNT','STTMINUS']

def build_images(w,pal,sc,atlas_rgb,atlas_alpha):
    """devolve dict de RGBA (para a quantizacao) e posicoes."""
    imgs={}
    imgs['atlas']=np.dstack([atlas_rgb,np.where(atlas_alpha,255,0).astype(np.uint8)])
    cells,winfo=build_sprite_cells(w,pal); imgs['sprites']=cells
    bar=to_rgba(w,w.picture('STBAR')[0],pal); imgs['bar']=scaled(bar,0.8)
    arms=to_rgba(w,w.picture('STARMS')[0],pal); imgs['arms']=scaled(arms,0.8)
    imgs['digits']=[scaled(to_rgba(w,w.picture(n)[0],pal),0.8) for n in DIGITS]
    imgs['faces']=[scaled(to_rgba(w,w.picture(n)[0],pal),0.8) for n in FACES]
    imgs['gun']=[]
    for n,s in GUN:
        idx,lo,to=w.picture(n); x1=1-lo; ytop=32-to; h,wd=idx.shape
        sc_=scaled(to_rgba(w,idx,pal),s); gh,gw=sc_.shape[:2]
        cx=128+((x1+wd/2)-160)*s; dx=int(round(cx-gw/2)); dy=int(round(178-(200-ytop)*s))
        imgs['gun'].append((sc_,dx,dy))
    imgs['fonts']={n:make_font(c) for n,c in FONT_COLS.items()}
    return imgs,winfo


GLYPHS={
'A':'01110 10001 10001 11111 10001 10001 10001','B':'11110 10001 10001 11110 10001 10001 11110','C':'01110 10001 10000 10000 10000 10001 01110',
'D':'11110 10001 10001 10001 10001 10001 11110','E':'11111 10000 10000 11110 10000 10000 11111','F':'11111 10000 10000 11110 10000 10000 10000',
'G':'01110 10001 10000 10111 10001 10001 01111','H':'10001 10001 10001 11111 10001 10001 10001','I':'01110 00100 00100 00100 00100 00100 01110',
'J':'00111 00010 00010 00010 00010 10010 01100','K':'10001 10010 10100 11000 10100 10010 10001','L':'10000 10000 10000 10000 10000 10000 11111',
'M':'10001 11011 10101 10101 10001 10001 10001','N':'10001 11001 10101 10011 10001 10001 10001','O':'01110 10001 10001 10001 10001 10001 01110',
'P':'11110 10001 10001 11110 10000 10000 10000','Q':'01110 10001 10001 10001 10101 10010 01101','R':'11110 10001 10001 11110 10100 10010 10001',
'S':'01111 10000 10000 01110 00001 00001 11110','T':'11111 00100 00100 00100 00100 00100 00100','U':'10001 10001 10001 10001 10001 10001 01110',
'V':'10001 10001 10001 10001 10001 01010 00100','W':'10001 10001 10001 10101 10101 10101 01010','X':'10001 10001 01010 00100 01010 10001 10001',
'Y':'10001 10001 01010 00100 00100 00100 00100','Z':'11111 00001 00010 00100 01000 10000 11111',
'0':'01110 10001 10011 10101 11001 10001 01110','1':'00100 01100 00100 00100 00100 00100 01110','2':'01110 10001 00001 00010 00100 01000 11111',
'3':'11110 00001 00001 01110 00001 00001 11110','4':'00010 00110 01010 10010 11111 00010 00010','5':'11111 10000 11110 00001 00001 10001 01110',
'6':'00110 01000 10000 11110 10001 10001 01110','7':'11111 00001 00010 00100 01000 01000 01000','8':'01110 10001 10001 01110 10001 10001 01110',
'9':'01110 10001 10001 01111 00001 00010 01100',' ':'00000 00000 00000 00000 00000 00000 00000','.':'00000 00000 00000 00000 00000 01100 01100',
'!':'00100 00100 00100 00100 00100 00000 00100','%':'11001 11010 00010 00100 01000 01011 10011','/':'00001 00010 00010 00100 01000 01000 10000',
'-':'00000 00000 00000 11111 00000 00000 00000',':':'00000 01100 01100 00000 01100 01100 00000'}
FONT_CHARS=' !./0123456789-ABCDEFGHIJKLMNOPQRSTUVWXYZ'
FONT_COLS={'font_gray':(180,164,156),'font_orange':(255,176,32),'font_red':(255,48,48)}
def make_font(col):
    img=np.zeros((7,6*len(FONT_CHARS),4),np.uint8)
    for i,ch in enumerate(FONT_CHARS):
        for y,row in enumerate(GLYPHS[ch].split()):
            for x,c in enumerate(row):
                if c=='1': img[y,i*6+x]=(*col,255)
    return img

def flat_color(w,name):
    if name=='F_SKY1': return (70,90,140)
    d=np.frombuffer(w.lump(name),np.uint8)[:4096]; pal=w.playpal(0); c=pal[d].astype(float).mean(0); return tuple(int(v) for v in c)

def main():
    L=P.load(sys.argv[1]); w=L.w; m=L.m; pal0=w.playpal(0)
    os.makedirs(PRIV,exist_ok=True)
    imgs,winfo=build_images(w,pal0,L.sc,L.rgb,L.alpha)
    # --- cores de piso/teto: 2 tons por flat usado (em setores abertos)
    used=sorted(set(m.sectors[s]['ftex'] for s in set(L.R.sec[L.R.open]))|set(m.sectors[s]['ctex'] for s in set(L.R.sec[L.R.open])))
    flat_cols={}
    for f in used: flat_cols[f]=flat_color(w,f)
    NF=len(used)*2
    fl_used=[f for f in used if f!='F_SKY1']; fl_id={f:i for i,f in enumerate(fl_used)}; assert len(fl_used)<=32
    def flat_tile(name):
        d=np.frombuffer(w.lump(name),np.uint8)[:4096].reshape(64,64); img=Image.fromarray(w.playpal(0)[d],'RGB').resize((16,16),Image.BOX)
        return np.dstack([np.array(img),np.full((16,16),255,np.uint8)])
    flat_tiles=[flat_tile(f) for f in fl_used]
    BO=2+NF; NBASE=256-BO                                          # 0 transparente; 1 = quase-preto (teto); 2..1+NF = tons dos flats; BO.. = cores base (indices < 128 ficam para cores LISAS de faces)
    # --- paleta: amostras de todas as imagens
    samples=[];wts=[]
    def add(rgba,wt):
        px=rgba.reshape(-1,4); px=px[px[:,3]>0][:,:3]; samples.append(px); wts.append(np.full(len(px),wt,np.float64))
    for t_ in flat_tiles: add(t_,3.0)
    add(imgs['atlas'],1.0); add(imgs['sprites'],4.0); add(imgs['bar'],3.0); add(imgs['arms'],2.0)
    for d in imgs['digits']: add(d,3.0)
    for d in imgs['faces']: add(d,3.0)
    for g,_,_ in imgs['gun']: add(g,3.0)
    for f in imgs['fonts'].values(): add(f,8.0)
    px=np.concatenate(samples); wt=np.concatenate(wts)
    u,inv=np.unique(px,axis=0,return_inverse=True); ww=np.bincount(inv,weights=wt)
    base=median_cut(u.astype(float),ww,NBASE)                       # (NBASE,3)
    # indice 0 = transparente (preto); 1..NBASE = base; NBASE+1.. = cores de piso/teto
    pal=np.zeros((256,3),float); pal[1]=(2,2,4); pal[BO:BO+len(base)]=base
    flat_idx={}
    for i,f in enumerate(used):
        c=np.array(flat_cols[f],float)
        for j,k in enumerate((0.8,1.0)):
            pal[2+2*i+j]=c*k
        flat_idx[f]=(2+2*i,3+2*i)             # (longe, perto)
    def q(rgba,dither=False):
        h,wd=rgba.shape[:2]; out=np.zeros((h,wd),np.uint8); f=rgba[...,:3].astype(float); b=base
        for y in range(h):
            for x in range(wd):
                if rgba[y,x,3]<128: continue
                old=f[y,x]; i=int((((b-old)**2*WEIGHT).sum(1)).argmin()); out[y,x]=i+BO
                if dither:
                    e=old-b[i]
                    for dx,dy,k in((1,0,7/16),(-1,1,3/16),(0,1,5/16),(1,1,1/16)):
                        xx,yy=x+dx,y+dy
                        if 0<=xx<wd and 0<=yy<h and rgba[yy,xx,3]>=128: f[yy,xx]+=e*k
        return out
    # --- VRAM: V = linhas 512..1023, B = linhas 468..491 (fontes)
    V=np.zeros((512,256),np.uint8)
    at=q(imgs['atlas'],dither=True); at[(at==0)]=1; V[0:at.shape[0]]=at
    V[256:384,0:240]=q(imgs['sprites'])                                   # linhas 768..895
    bar=q(imgs['bar']); V[384:384+bar.shape[0],0:bar.shape[1]]=bar    # 896..
    layout={}
    layout['bar']=(0,896,bar.shape[1],bar.shape[0])
    arms=q(imgs['arms']); ay=896+bar.shape[0]; V[ay-512:ay-512+arms.shape[0],0:arms.shape[1]]=arms; layout['arms']=(0,ay,arms.shape[1],arms.shape[0])
    x=40; dig=[]
    for d in imgs['digits']:
        qd=q(d); V[ay-512:ay-512+qd.shape[0],x:x+qd.shape[1]]=qd; dig.append((x,ay,qd.shape[1],qd.shape[0])); x+=qd.shape[1]
    layout['digits']=dig
    fy=ay+14; x=40; fc=[]
    for f in imgs['faces']:
        qf=q(f); V[fy-512:fy-512+qf.shape[0],x:x+qf.shape[1]]=qf; fc.append((x,fy,qf.shape[1],qf.shape[0])); x+=qf.shape[1]+1
    layout['faces']=fc
    gy=max(fy+max(f.shape[0] for f in imgs['faces'])+1,ay+arms.shape[0]+1); x=0; gn=[]
    assert gy+53<=1024,gy
    for g,dx,dy in imgs['gun']:
        qg=q(g); gh=qg.shape[0]; V[gy-512:gy-512+gh,x:x+qg.shape[1]]=qg; gn.append((x,gy,qg.shape[1],gh,dx,dy)); x+=qg.shape[1]+1
    assert x<=256,x
    layout['gun']=gn
    B=np.zeros((64,256),np.uint8)                                   # linhas 468..531: fontes em 468..
    chars=FONT_CHARS; fyb=468; fo={}
    for n,f in imgs['fonts'].items():
        qf=q(f); B[fyb-468:fyb-468+7,0:qf.shape[1]]=qf; fo[n]=fyb; fyb+=8
    layout['fonts']=fo
    # --- bancos
    A=np.zeros((64,256),np.uint8)                                    # linhas 212..275: tiles de piso/teto 16x16 (16 por linha de tiles)
    for k,t_ in enumerate(flat_tiles): qt=q(t_); qt[qt==0]=1; A[16*(k//16):16*(k//16)+16,16*(k%16):16*(k%16)+16]=qt
    banks=[A.tobytes(),B.tobytes()]                                  # banco 2: area A (tiles); banco 3: area B (fontes)
    for i in range(8): banks.append(V[i*64:(i+1)*64].tobytes())      # bancos 3..10
    # --- paleta (5 bits/canal)
    pal5=np.clip(np.rint(pal/255*31),0,31).astype(np.uint8)
    # --- setores abertos usados: reindexa
    sec_ids=sorted(set(int(s) for s in L.R.sec[L.R.open]))
    smap={s:i for i,s in enumerate(sec_ids)}; assert len(sec_ids)<255
    sectors=[]
    for s in sec_ids:
        sd=m.sectors[s]; fi=flat_idx[sd['ftex']]; ci=flat_idx[sd['ctex']]
        sky=sd['ctex']=='F_SKY1'
        sectors.append((sd['floor'],sd['ceil'],fi[0],fi[1],ci[0],ci[1],sd['special'],1 if sky else 0,fl_id[sd['ftex']],255 if sky else fl_id[sd['ctex']]))
    # --- raster de setores (255 = bloqueado)
    R=L.R; rast=np.full((R.ny,R.nx),255,np.uint8)
    ys,xs=np.nonzero(R.open); 
    for y,x in zip(ys,xs): rast[y,x]=smap[int(R.sec[y,x])]
    ROWS=16384//R.nx; nrb=(R.ny+ROWS-1)//ROWS; RAST_BANK0=len(banks)+2+1   # +2: bancos 0,1 do mkrom; +1: banco meta
    meta_bank=len(banks)+2
    # --- blocos
    bl=P.open_blocks(L); BX0=min(b[0] for b in bl); BY0=min(b[1] for b in bl); BNX=max(b[0] for b in bl)-BX0+1; BNY=max(b[1] for b in bl)-BY0+1
    # --- itens/monstros
    things=[]
    SK=SKILL_FLAG
    KIND={3004:0,9:1,3001:2,2011:3,2012:4,2014:5,2015:6,2018:7,2019:8,2007:9,2048:10,2008:11,2049:12,2001:13,2035:14,2028:15,48:16,35:17}
    SPR_OF={0:'POSSA1',1:'SPOSA1',2:'TROOA1',3:'STIMA0',4:'MEDIA0',5:'BON1A0',6:'BON2A0',7:'ARM1A0',8:'ARM2A0',9:'CLIPA0',10:'AMMOA0',11:'SHELA0',12:'SBOXA0',13:'SHOTA0',14:'BAR1A0',15:'COLUA0',16:'ELECA0',17:'CBRAA0'}
    for t in m.things:
        if t['type'] not in KIND or not (t['flags']&SK) or (t['flags']&16): continue
        cx,cy=int((t['x']-L.R.x0)//8),int((t['y']-L.R.y0)//8)
        if not L.R.open[cy-1:cy+2,cx-1:cx+2].any(): print('ignorado (fora da area acessivel):',t); continue
        things.append((t['x'],t['y'],KIND[t['type']],t['angle']))
    start=[t for t in m.things if t['type']==1][0]
    ex=[l for l in m.linedefs if l['special']==11][0]; (e1x,e1y),(e2x,e2y)=m.vertexes[ex['v1']],m.vertexes[ex['v2']]
    # --- blobs
    cur=bytearray(); blobbanks=[]; blk_tab={}
    nblob=0; maxf=0; seen={}; pend=[]
    first_blob_bank=RAST_BANK0+nrb
    import time; t0=time.time(); res={}
    for i,(bx,by) in enumerate(bl):
        vps=P.block_viewpoints(L,bx,by)
        if not vps: continue
        allids=sorted(P.visible_subs(L,vps)); allt=P.visible_tiles(L,vps,bx,by)
        for hd in range(P.NHEAD):
            ids=[j for j in allids if P.inview(L,j,bx,by,P.head_deg(hd),P.CONE)]
            r=P.build_block(L,bx,by,ids=ids,tiles=[t for t in allt if P.tile_in_cone(L,t,bx,by,P.head_deg(hd),P.CONE)])
            res[(bx,by,hd)]=r; maxf=max(maxf,len(r['faces']))
            nv=len(r['verts']); nf=len(r['faces'])
            blob=bytes([nv,nf])+b''.join(struct.pack('<hhh',*v) for v in r['verts'])
            def fcol(fl): return 0x80 if fl is None else (1 if fl[0]=='C' else flat_idx[m.sectors[fl[1]]['ftex']][1])
            for fidx,fl in zip(r['faces'],r['flat']): blob+=bytes(fidx)+struct.pack('<hhhB',0,-16384,0,fcol(fl))
            for uv in r['uvs']: blob+=bytes(uv)
            if blob in seen: blk_tab[(bx,by,hd)]=blob; continue
            seen[blob]=None; pend.append((blob,(bx,by,hd))); blk_tab[(bx,by,hd)]=blob
        if i%20==0: print('bloco',i,len(bl),'%.0fs'%(time.time()-t0),flush=True)
    # empacotamento first-fit decrescente (menos desperdicio nos bancos)
    place={}; bank_used=[]; bank_data=[]
    for blob in sorted(set(b for b,_ in pend),key=len,reverse=True):
        for bi,u in enumerate(bank_used):
            if u+len(blob)<=16384: place[blob]=(first_blob_bank+bi,u); bank_data[bi]+=blob; bank_used[bi]+=len(blob); break
        else:
            place[blob]=(first_blob_bank+len(bank_used),0); bank_used.append(len(blob)); bank_data.append(bytearray(blob))
    for k_,b_ in list(blk_tab.items()): blk_tab[k_]=place[b_]
    blobbanks=[bytes(b).ljust(16384,b'\0') for b in bank_data]
    print('blobs (bancos de 16K):',len(blobbanks),'| bancos antes dos blobs:',first_blob_bank,'| total',first_blob_bank+len(blobbanks))
    if first_blob_bank+len(blobbanks)>256: print('ERRO: ROM passaria de 256 bancos'); sys.exit(1)
    # indice de blocos
    keys=sorted(blk_tab)                                      # (bx,by,heading); indice = bloco*4 + direcao
    blocks_=sorted(set((k[0],k[1]) for k in keys)); bidx={b:i for i,b in enumerate(blocks_)}
    keys=[(b[0],b[1],hd) for b in blocks_ for hd in range(P.NHEAD)]
    grid=[0xFFFF]*(BNX*BNY)
    for b,i in bidx.items(): grid[(b[1]-BY0)*BNX+(b[0]-BX0)]=i
    # --- meta: sin (512), setores (8 B cada), blocos (3 B cada), grade (2 B por celula), paleta (768)
    meta=bytearray()
    off={}
    off['sin']=len(meta); meta+=b''.join(struct.pack('<h',int(round(math.sin(i*2*math.pi/256)*16384))) for i in range(256))
    off['pal']=len(meta); meta+=bytes(pal5.reshape(-1).tolist())
    off['sec']=len(meta); meta+=b''.join(struct.pack('<hhBBBBBBBB',*s) for s in sectors)
    off['blk']=len(meta); meta+=b''.join(struct.pack('<BH',*blk_tab[k]) for k in keys)
    off['grid']=len(meta); meta+=b''.join(struct.pack('<H',g) for g in grid)
    off['thing']=len(meta); meta+=b''.join(struct.pack('<hhBh',*t) for t in things)
    assert len(meta)<=16384,len(meta)
    meta_b=bytes(meta).ljust(16384,b'\0')
    rb=[]
    for k in range(nrb):
        part=rast[k*ROWS:(k+1)*ROWS].tobytes(); rb.append(part.ljust(16384,b'\0'))
    if first_blob_bank+len(blobbanks)>256: print('ERRO: ROM passaria de 256 bancos'); sys.exit(1)
    allb=banks+[meta_b]+rb+blobbanks
    open(os.path.join(PRIV,'e1m1_banks.bin'),'wb').write(b''.join(allb))
    # --- cabecalho C
    h=['/* GERADO por tools/e1m1_build.py a partir do WAD do usuario - PRIVADO, nao versionar */']
    h+=['#define AREAA_BANK 2','#define AREAB_BANK 3','#define VRAM_BANK0 4','#define FLAT_TILE_Y 212','#define FL_REG_Y 640','#define CE_REG_Y 704','#define NVRAM_BANKS 8','#define META_BANK %d'%meta_bank,'#define RAST_BANK0 %d'%RAST_BANK0,
        '#define RAST_W %d'%R.nx,'#define RAST_H %d'%R.ny,'#define RAST_ROWS %d'%ROWS,'#define RAST_X0 %d'%R.x0,'#define RAST_Y0 %d'%R.y0,
        '#define BX0 %d'%BX0,'#define BY0 %d'%BY0,'#define BNX %d'%BNX,'#define BNY %d'%BNY,'#define NSEC %d'%len(sectors),'#define NBLK %d'%len(blocks_),'#define NTHING %d'%len(things)]
    for k,v in off.items(): h.append('#define META_%s %d'%(k.upper(),v))
    h+=['#define START_X %d'%start['x'],'#define START_Y %d'%start['y'],'#define START_ANG %d'%start['angle'],'#define EXIT_X %d'%((e1x+e2x)//2),'#define EXIT_Y %d'%((e1y+e2y)//2)]
    mk=sum(1 for t in things if t[2]<=2)
    h.append('#define NMON %d'%mk)
    h.append('#define SPR_COLS 10'); h.append('#define SPR_CW %d\n#define SPR_CH %d'%(CW,CH))
    h.append('const unsigned char spr_scale[%d]={'%len(SPRITES)+','.join('%d'%round(s*4) for s in winfo)+'};   /* unidades por texel x4 */')
    for n,i in SLOT.items(): h.append('#define SPR_%s %d'%(n,i))
    h.append('const unsigned char kind_spr[18]={'+','.join(str(SLOT[SPR_OF[k]]) for k in range(18))+'};')
    bx_,by_,bw,bh=layout['bar']; h.append('#define BAR_X %d\n#define BAR_Y %d\n#define BAR_W %d\n#define BAR_H %d'%layout['bar'])
    h.append('#define ARMS_Y %d\n#define ARMS_W %d\n#define ARMS_H %d'%(layout['arms'][1],layout['arms'][2],layout['arms'][3]))
    h.append('const unsigned int digit_tab[12][4]={'+','.join('{%d,%d,%d,%d}'%d for d in layout['digits'])+'};')
    h.append('const unsigned int face_tab[6][4]={'+','.join('{%d,%d,%d,%d}'%d for d in layout['faces'])+'};')
    h.append('const int gun_tab[5][6]={'+','.join('{%d,%d,%d,%d,%d,%d}'%g for g in layout['gun'])+'};')
    for nm,col in (('WHITE',(255,255,255)),('RED',(255,48,48)),('PANEL',(28,24,40))):
        h.append('#define C_%s %d'%(nm,int((((base-np.array(col))**2*WEIGHT).sum(1)).argmin())+BO))
    h.append('#define C_BLACK 1')
    h.append('#define FONT_CHARS "%s"'%chars)
    for n,v in layout['fonts'].items(): h.append('#define %s %d'%(n.upper(),v))
    open(os.path.join(PRIV,'e1m1_assets.h'),'w').write('\n'.join(h)+'\n')
    # --- previews
    rgb=lambda ix:np.array([[tuple(int(c)*255//31 for c in pal5[i]) for i in row] for row in ix],np.uint8)
    Image.fromarray(rgb(V)).save(os.path.join(PRIV,'vram_512_1023.png'))
    print('bancos de dados',len(allb),'| blocos',len(keys),'| faces max',maxf,'| paleta base',len(base),'+',NF,'| coisas',len(things),'monstros',mk,'| setores',len(sectors),'| raster bancos',nrb)

if __name__=='__main__': main()
