"""Leitor de WAD do Doom (formato publico documentado: cabecalho, diretorio de lumps, mapas, texturas, imagens).
Le o doom1.wad DO USUARIO, localmente. Nada do WAD (nem dados convertidos dele) deve ir para o repositorio.
Uso como biblioteca: w=Wad(caminho); w.load_map('E1M1'); w.texture('STARTAN3'); w.sprite('POSSA1')."""
import struct,os,collections
import numpy as np

class Wad:
    def __init__(s,path):
        s.data=open(path,'rb').read()
        ident,n,off=struct.unpack('<4sii',s.data[:12]); assert ident in (b'IWAD',b'PWAD')
        s.lumps=[]
        for i in range(n):
            p,sz,nm=struct.unpack('<ii8s',s.data[off+16*i:off+16*i+16]); s.lumps.append((nm.rstrip(b'\0').decode('latin1').upper(),p,sz))
        s.index={}
        for i,(nm,_,_) in enumerate(s.lumps): s.index[nm]=i                 # o ultimo lump de um nome vence
        s._tex=None
    def lump(s,name):
        _,p,sz=s.lumps[s.index[name.upper()]]; return s.data[p:p+sz]
    def has(s,name): return name.upper() in s.index
    # ---------------------------------------------------------------- paleta
    def playpal(s,n=0):
        d=s.lump('PLAYPAL')[768*n:768*(n+1)]; return np.frombuffer(d,np.uint8).reshape(256,3)
    # ---------------------------------------------------------------- imagens (formato "picture")
    def picture(s,name):
        """Devolve (indices HxW int16 com -1 = transparente, leftoffset, topoffset)."""
        d=s.lump(name); w,h,lo,to=struct.unpack('<hhhh',d[:8])
        cols=struct.unpack('<%di'%w,d[8:8+4*w]); img=-np.ones((h,w),np.int16)
        for x,co in enumerate(cols):
            p=co
            while True:
                top=d[p]
                if top==0xFF: break
                ln=d[p+1]; px=d[p+3:p+3+ln]
                for k in range(ln):
                    y=top+k
                    if 0<=y<h: img[y,x]=px[k]
                p+=ln+4
        return img,lo,to
    def sprite(s,name): return s.picture(name)
    # ---------------------------------------------------------------- texturas de parede
    def _load_textures(s):
        pn=s.lump('PNAMES'); n=struct.unpack('<i',pn[:4])[0]
        s.pnames=[pn[4+8*i:12+8*i].rstrip(b'\0').decode('latin1').upper() for i in range(n)]
        s._tex={}
        for lname in ('TEXTURE1','TEXTURE2'):
            if not s.has(lname): continue
            d=s.lump(lname); cnt=struct.unpack('<i',d[:4])[0]
            offs=struct.unpack('<%di'%cnt,d[4:4+4*cnt])
            for o in offs:
                nm=d[o:o+8].rstrip(b'\0').decode('latin1').upper()
                w,h=struct.unpack('<hh',d[o+12:o+16]); pc=struct.unpack('<h',d[o+20:o+22])[0]; patches=[]
                for k in range(pc):
                    ox,oy,pi,_,_=struct.unpack('<hhhhh',d[o+22+10*k:o+32+10*k]); patches.append((ox,oy,s.pnames[pi]))
                s._tex[nm]=(w,h,patches)
    def texture_info(s,name):
        if s._tex is None: s._load_textures()
        return s._tex[name.upper()]
    def texture(s,name):
        """Textura composta: indices HxW (-1 = transparente), com os remendos desenhados em ordem."""
        w,h,patches=s.texture_info(name); out=-np.ones((h,w),np.int16)
        for ox,oy,pn in patches:
            img,_,_=s.picture(pn); ph,pw=img.shape
            for y in range(ph):
                yy=y+oy
                if not 0<=yy<h: continue
                x0=max(0,-ox); x1=min(pw,w-ox)
                if x1<=x0: continue
                row=img[y,x0:x1]; m=row>=0
                out[yy,ox+x0:ox+x1][m]=row[m]
        return out
    # ---------------------------------------------------------------- mapas
    def load_map(s,name):
        i=s.index[name.upper()]; L={nm:(p,sz) for nm,p,sz in s.lumps[i+1:i+11]}
        def rd(nm,fmt):
            p,sz=L[nm]; sl=struct.calcsize(fmt); return [struct.unpack(fmt,s.data[p+k*sl:p+(k+1)*sl]) for k in range(sz//sl)]
        m=types_ns()
        m.vertexes=rd('VERTEXES','<hh')
        # lados: indice sem sinal; 0xFFFF = sem lado (linha de um lado so) -> guardamos -1
        m.linedefs=[dict(v1=a,v2=b,flags=f,special=sp,tag=t,right=r if r!=0xFFFF else -1,left=l if l!=0xFFFF else -1) for a,b,f,sp,t,r,l in rd('LINEDEFS','<HHhhhHH')]
        m.sidedefs=[dict(xoff=a,yoff=b,upper=u.rstrip(b'\0').decode('latin1'),lower=lo.rstrip(b'\0').decode('latin1'),
                         mid=mi.rstrip(b'\0').decode('latin1'),sector=sc) for a,b,u,lo,mi,sc in rd('SIDEDEFS','<hh8s8s8sh')]
        m.sectors=[dict(floor=f,ceil=c,ftex=ft.rstrip(b'\0').decode('latin1'),ctex=ct.rstrip(b'\0').decode('latin1'),light=li,special=sp,tag=tg)
                   for f,c,ft,ct,li,sp,tg in rd('SECTORS','<hh8s8shhh')]
        m.things=[dict(x=x,y=y,angle=a,type=t,flags=f) for x,y,a,t,f in rd('THINGS','<hhhhh')]
        return m

class types_ns: pass

if __name__=='__main__':
    import sys
    w=Wad(sys.argv[1]); m=w.load_map('E1M1')
    xs=[v[0] for v in m.vertexes]; ys=[v[1] for v in m.vertexes]
    print('vertices',len(m.vertexes),'linedefs',len(m.linedefs),'sidedefs',len(m.sidedefs),'setores',len(m.sectors),'things',len(m.things))
    print('extensao x',min(xs),max(xs),'y',min(ys),max(ys))
