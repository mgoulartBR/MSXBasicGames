"""Gera os dados da ROM texturizada: paleta EPAL de 256 cores, imagem de VRAM (linhas 512..1023 em
SCREEN 8), bancos de dados ASCII16 e src/assets.h.

Layout da VRAM (SCREEN 8: 256 bytes por linha, 256 KiB no total, páginas 0/1 em 0..511):
  512..575  parede, cópia nível 0 (escura)       TEXY=512, TSTRIDE=64
  576..639  atlas de sprites 256x64 (sem sombra)
  640..703  parede, cópia nível 2 (média)
  704..767  (livre)
  768..831  parede, cópia nível 4 (clara)
  832..895  arma (idle em x=0, fogo em x=64), clarão (x=128, 48x32)
  896..1023 fontes
O Geo3D lê a linha TEXY + nível*TSTRIDE + v; a luz fixa no mundo só gera os níveis 0, 2 e 4 nas paredes."""
import math,os,struct,sys
import numpy as np
from PIL import Image
HERE=os.path.dirname(os.path.abspath(__file__)); ROOT=os.path.join(HERE,'..')
sys.path.insert(0,HERE)
import level
A=os.path.join(ROOT,'assets')
def load(n):return np.array(Image.open(os.path.join(A,n+'.png')).convert('RGBA'),dtype=np.uint8)

# ---------------------------------------------------------------- cores
FORCED=[(180,164,156),(255,176,32),(255,48,48),(255,255,255)]        # fontes cinza/laranja/vermelho, mira
NBASE=63
def median_cut(cols,w,k):
    boxes=[np.arange(len(cols))]
    while len(boxes)<k:
        best,bs=-1,-1
        for i,b in enumerate(boxes):
            if len(b)<2:continue
            rng=(cols[b].max(0)-cols[b].min(0)).max(); s=rng*w[b].sum()
            if s>bs:bs,best=s,i
        if best<0:break
        b=boxes.pop(best); ch=int(np.argmax(cols[b].max(0)-cols[b].min(0)))
        o=b[np.argsort(cols[b][:,ch])]; cw=np.cumsum(w[o]); h=int(np.searchsorted(cw,cw[-1]/2))+1; h=min(max(h,1),len(o)-1)
        boxes+= [o[:h],o[h:]]
    cen=np.array([(cols[b]*w[b,None]).sum(0)/w[b].sum() for b in boxes])
    for _ in range(10):                                              # refinamento k-means ponderado
        d=((cols[:,None,:]-cen[None])**2*np.array([2,4,3])).sum(2); lab=d.argmin(1)
        for j in range(len(cen)):
            m=lab==j
            if m.any():cen[j]=(cols[m]*w[m,None]).sum(0)/w[m].sum()
    return cen
def build_palette(images):
    acc={}
    for arr,wt in images:
        px=arr.reshape(-1,4); px=px[px[:,3]>0][:,:3]
        u,c=np.unique(px,axis=0,return_counts=True)
        for col,n in zip(map(tuple,u),c):acc[col]=acc.get(col,0)+n*wt
    cols=np.array(list(acc.keys()),dtype=float); w=np.array(list(acc.values()),dtype=float)
    cen=median_cut(cols,w,NBASE-len(FORCED))
    base=[np.array(f,float) for f in FORCED]+list(cen)
    return np.array(base)                                            # (63,3)
WEIGHT=np.array([2,4,3])
def nearest(rgb,base):return (((rgb[:,None,:]-base[None])**2*WEIGHT).sum(2)).argmin(1)+1   # índice base 1..63
def quantize(arr,base,dither=False):
    h,w,_=arr.shape; out=np.zeros((h,w),np.uint8); f=arr[:,:,:3].astype(float).copy()
    for y in range(h):
        for x in range(w):
            if arr[y,x,3]<128:out[y,x]=0;continue
            old=f[y,x]; i=int(nearest(old[None],base)[0]); out[y,x]=i
            if dither:
                e=old-base[i-1]
                for dx,dy,k in((1,0,7/16),(-1,1,3/16),(0,1,5/16),(1,1,1/16)):
                    xx,yy=x+dx,y+dy
                    if 0<=xx<w and 0<=yy<h and arr[yy,xx,3]>=128:f[yy,xx]+=e*k
    return out
BR=[(1.0,0),(0.66,64),(0.36,128)]                                    # (brilho, deslocamento do índice) por cópia
def _ramp(a,b,n):return [tuple(round(a[k]+(b[k]-a[k])*i/(n-1)) for k in range(3)) for i in range(n)]
FLAT={}
for i,c in enumerate(_ramp((36,24,22),(72,52,44),6)):FLAT['ceil%d'%i]=c       # ceil0 = junto ao horizonte, ceil5 = topo
for i,c in enumerate(_ramp((44,32,30),(104,82,72),8)):FLAT['floor%d'%i]=c     # floor0 = horizonte, floor7 = perto
FLAT.update(hud_bg=(22,10,8),hud_border=(106,64,56),bar_bg=(64,16,16),bar_red=(232,32,32),cross_red=(255,60,60),black=(0,0,0))
FLAT_IDX={n:192+i for i,n in enumerate(FLAT)}
def epal(base):
    pal=np.zeros((256,3),float)
    for b in range(1,64):
        for br,off in BR:pal[off+b]=base[b-1]*br
    for n,i in FLAT_IDX.items():pal[i]=FLAT[n]
    return np.clip(np.rint(pal/255*31),0,31).astype(np.uint8)         # 5 bits por canal

def main():
    names=['stone','stoneG','flesh','acid']
    tiles={n:load(n) for n in names}
    poses=['w0','w1','a','hurt','d0','d1']
    sprites=[(t,p) for t in('imp','zom') for p in poses]
    spr={f'{t}_{p}':load(f'{t}_{p}') for t,p in sprites}
    gun={n:load(n) for n in('gun_idle','gun_fire','flash')}
    fonts={n:load(n) for n in('font_gray','font_orange','font_red','font_red2')}
    medkit=load('medkit')
    imgs=[(tiles[n],3) for n in names]+[(medkit,5)]+[(spr[k],5) for k in spr]+[(gun[k],3) for k in gun]
    base=build_palette(imgs)
    pal=epal(base)
    # --- imagem de VRAM, linhas 512..1023
    V=np.zeros((512,256),np.uint8)
    atlas=np.zeros((64,256),np.uint8)
    for i,n in enumerate(names):atlas[:,i*64:(i+1)*64]=quantize(tiles[n],base,dither=True)
    # níveis: 0 -> escura (off 128), 2 -> média (off 64), 4 -> clara (off 0)
    for level_,off,row in((0,128,0),(2,64,128),(4,0,256)):
        V[row:row+64]=np.where(atlas>0,atlas.astype(int)+off,0).astype(np.uint8)
    sa=np.zeros((64,256),np.uint8)
    for k,(t,p) in enumerate(sprites):
        x,y=(k%10)*24,(k//10)*32; sa[y:y+32,x:x+24]=quantize(spr[f'{t}_{p}'],base)
    sa[32:64,48:72]=quantize(medkit,base)                              # sprite 12 = kit médico
    V[64:128]=sa                                                       # linhas 576..639
    V[320:320+64,0:64]=quantize(gun['gun_idle'],base); V[320:384,64:128]=quantize(gun['gun_fire'],base)   # linhas 832..
    V[320:352,128:176]=quantize(gun['flash'],base)
    chars=open(os.path.join(A,'font_chars.txt')).read()
    fy=384                                                             # linha 896
    for n in('font_gray','font_orange','font_red'):
        q=quantize(fonts[n],base); V[fy:fy+7,0:q.shape[1]]=q; fy+=8
    big=fonts['font_red2'][:,:12*12]; q=quantize(big,base); V[fy:fy+14,0:q.shape[1]]=q
    layout=dict(wall=512,sprites=576,gun_idle=(0,832),gun_fire=(64,832),flash=(128,832),
                font_gray=896,font_orange=904,font_red=912,font_red2=920,chars=chars)
    # --- bancos de dados: 0=boot,1=código (preenchidos pelo mkrom); aqui 2..9 = VRAM, 10+ = nível
    banks=[V[i*64:(i+1)*64].tobytes() for i in range(8)]
    # --- nível texturizado
    import pvs
    blocks=pvs.build_all()
    cur=bytearray(); levelbanks=[]; blk_tab={}
    for (bx,bz),c in sorted(blocks.items(),key=lambda e:(e[0][1],e[0][0])):
        blob=c['blob']
        if len(cur)+len(blob)>16384:levelbanks.append(bytes(cur).ljust(16384,b'\0'));cur=bytearray()
        blk_tab[bz*pvs.BW+bx]=(10+len(levelbanks),len(cur)); cur+=blob
    levelbanks.append(bytes(cur).ljust(16384,b'\0'))
    banks+=levelbanks
    open(os.path.join(ROOT,'out','data_banks.bin'),'wb').write(b''.join(banks))
    # --- assets.h
    h=['/* GERADO por tools/make_assets.py - não editar */','#define NBLOCK %d'%(pvs.BW*pvs.BH),'#define NBX %d'%pvs.BW,'#define MAX_LV %d'%pvs.MAX_VERTS,'#define MAX_LF %d'%pvs.MAX_FACES,'#define FIRST_VRAM_BANK 2','#define NVRAM_BANKS 8']
    for n,i in FLAT_IDX.items():h.append('#define C_%s %d'%(n.upper(),i))
    h.append('#define C_WHITE 4')
    h.append('typedef struct{unsigned char bank;unsigned int off;}Block;   /* PVS do bloco 2x2: no blob, byte 0 = nv, byte 1 = nf, depois vertices, faces e UVs */')
    h.append('const Block blocks[NBLOCK]={'+','.join('{%d,%d}'%blk_tab.get(i,(0,0)) for i in range(pvs.BW*pvs.BH))+'};')
    pb=pal.reshape(-1).tolist(); h.append('const unsigned char palette[768]={'+','.join(map(str,pb))+'};')
    sin=[int(round(math.sin(i*2*math.pi/256)*16384)) for i in range(256)]
    h.append('const int sin_tab[256]={'+','.join(map(str,sin))+'};')
    P,E,Hs,X=level.entities()
    h.append('#define START_X %d\n#define START_Z %d\n#define NENEMY %d\n#define NPICK %d\n#define EXIT_CELL %d'%(P[0],P[1],len(E),len(Hs),(X[1]<<5)+X[0]))
    h.append('const int enemy_start[NENEMY][2]={'+','.join('{%d,%d}'%e for e in E)+'};')
    h.append('const int pick_start[NPICK][2]={'+','.join('{%d,%d}'%e for e in Hs)+'};')
    def bits(fn):
        b=bytearray(128)
        for z in range(level.HH):
            for x in range(level.W):
                if fn(x,z): i=(z<<5)+x; b[i>>3]|=1<<(i&7)
        return b
    h.append('#define MAPW %d\n#define MAPH %d'%(level.W,level.HH))
    h.append('/* bit (cz<<5)+cx */\nconst unsigned char map_solid[128]={'+','.join(map(str,bits(level.solid)))+'};')
    h.append('const unsigned char map_acid[128]={'+','.join(map(str,bits(level.acid)))+'};')
    # índices de glifo
    h.append('#define FONT_CHARS "%s"'%chars)
    open(os.path.join(ROOT,'src','assets.h'),'w').write('\n'.join(h)+'\n')
    # --- pré-visualizações
    os.makedirs(os.path.join(ROOT,'out'),exist_ok=True)
    rgb=lambda ix:np.array([[tuple(int(c)*255//31 for c in pal[i]) for i in row] for row in ix],np.uint8)
    Image.fromarray(rgb(V)).save(os.path.join(ROOT,'out','vram_512_1023.png'))
    print('banks:',len(banks)+2,'(0=boot,1=codigo) | blocos',len(blocks),'| faces/bloco max',max(len(c['faces']) for c in blocks.values()),'| base',len(base))

if __name__=='__main__':main()
