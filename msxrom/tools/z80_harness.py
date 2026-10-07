"""Executa out/msxdoom.rom (ASCII16) num emulador de Z80 (pip z80) com um MODELO SIMPLIFICADO de:
  - mapper ASCII16 (bancos em 0x6000 / 0x7000)
  - V9968 em SCREEN 8: VRAM de 256 KiB, paleta EPAL (R#20 bit4), LMMV, LMMM (com TIMP), páginas via R#2
  - Geo3D (registradores de geo3d_engine.v): faces preenchidas e TEXTURADAS (LRMM por linha, afim, cópia por nível)
Validação classe SIMULATED: não é openMSX/FPGA/hardware. Uso: z80_harness.py [frames] [prefixo_saida]"""
import sys,struct,os
import numpy as np
from z80 import Z80Machine
from PIL import Image
ROM=os.path.join(os.path.dirname(__file__),'..','out','msxdoom.rom')

class Machine:
    def __init__(s,keys_fn,rom=ROM):
        s.m=Z80Machine(); mem=s.m.memory
        s.rom=open(rom,'rb').read(); s.banks=[s.rom[i:i+16384] for i in range(0,len(s.rom),16384)]
        s.win1=0; s.win2=0
        mem[0x4000:0x8000]=s.banks[0]; mem[0x8000:0xC000]=s.banks[0]
        for a in (0x0138,0x0024,0x0180,0x005F): mem[a]=0xC9            # stubs de BIOS: RSLREG, ENASLT, CHGCPU
        mem[0x2D]=3                                                    # turbo R
        s.m.pc=0x4002 if False else (s.banks[0][2]|s.banks[0][3]<<8); s.m.sp=0xF380
        s.m.mark_addrs(0x6000,0x800,s.m.WRITE_MARK); s.m.mark_addrs(0x7000,0x800,s.m.WRITE_MARK)
        s.m.set_write_callback(s.wr)
        # --- V9968
        s.vram=np.zeros(1<<18,np.uint8); s.pal=np.zeros((256,3),np.uint8); s.vreg=[0]*64
        s.vlatch=None; s.ind=0; s.vaddr=0; s.pal_i=0; s.pal_ph=0; s.pal_cur=[0,0,0]
        # --- Geo3D
        s.gi=0; s.gb=bytearray(256); s.gv={}; s.gf={}; s.gt={}; s.gbuf={0x50:bytearray(),0x52:bytearray(),0x53:bytearray()}
        s.keys_fn=keys_fn; s.ppi=0; s.frames=[]
        s.log={'geo_runs':0,'lmmv':0,'lmmm':0,'page_flips':0,'faces':0,'tex_pixels':0,'geo_bytes':0}
        s.m.set_input_callback(s.inp); s.m.set_output_callback(s.out)
    # ---- mapper
    def wr(s,addr,val):
        mem=s.m.memory
        if 0x6000<=addr<0x6800: s.win1=val%len(s.banks); mem[0x4000:0x8000]=s.banks[s.win1]
        elif 0x7000<=addr<0x7800: s.win2=val%len(s.banks); mem[0x8000:0xC000]=s.banks[s.win2]
    # ---- entrada
    def inp(s,port):
        port&=0xFF
        if port==0x99: return 0x80 if s.vreg[15]==0 else 0x00         # S#0 vblank / S#2 CE=0
        if port==0x9D: return 0
        if port==0xA9: return s.keys_fn(s.ppi&0x0F)
        return 0xFF
    # ---- saída
    def out(s,port,v):
        port&=0xFF; v&=0xFF
        if port==0x98:
            s.vram[s.vaddr&0x3FFFF]=v; s.vaddr+=1
        elif port==0x99:
            if s.vlatch is None: s.vlatch=v
            else:
                if v&0x80: s.wreg(v&0x3F,s.vlatch)
                else: s.vaddr=((s.vreg[14]&15)<<14)|((v&0x3F)<<8)|s.vlatch
                s.vlatch=None
        elif port==0x9A:
            s.pal_cur[s.pal_ph]=v&31; s.pal_ph+=1
            if s.pal_ph==3:
                s.pal[s.pal_i&255]=[c*255//31 for c in s.pal_cur]; s.pal_i+=1; s.pal_ph=0
        elif port==0x9B:
            s.vreg[s.ind]=v
            if s.ind==46: s.exec_vdp()
            if not (s.vreg[17]&0x80): s.ind=(s.ind+1)&63
        elif port==0x9D: s.gi=v
        elif port==0x9F: s.gwrite(v)
        elif port==0xAA: s.ppi=v
    def wreg(s,r,v):
        if r==17: s.ind=v&0x3F
        elif r==16: s.pal_i=v; s.pal_ph=0
        else:
            s.vreg[r]=v
            if r==2: s.page_flip(v)
        if r==17: s.vreg[17]=v
    def snapshot(s,page):
        img=s.vram[page*65536:page*65536+212*256].reshape(212,256); return Image.fromarray(s.pal[img],'RGB')
    def page_flip(s,v):
        s.log['page_flips']+=1; s.frames.append(s.snapshot((v>>5)&3))
    def exec_vdp(s):
        r=s.vreg; cmd=r[46]>>4; lop=r[46]&15
        g=lambda lo:r[lo]|(r[lo+1]&3)<<8
        if cmd in (8,12):                                            # LMMV / HMMV (SCREEN 8: 1 byte por pixel)
            x,y,w,h=r[36]|(r[37]&1)<<8,g(38),r[40]|(r[41]&1)<<8,g(42)
            for yy in range(y,min(y+h,1024)):
                s.vram[yy*256+x:yy*256+min(x+w,256)]=r[44]
            s.log['lmmv']+=1
        elif cmd==9:                                                 # LMMM
            sx,sy,dx,dy,w,h=r[32]|(r[33]&1)<<8,g(34),r[36]|(r[37]&1)<<8,g(38),r[40]|(r[41]&1)<<8,g(42)
            for j in range(h):
                if sy+j>=1024 or dy+j>=1024:break
                src=s.vram[(sy+j)*256+sx:(sy+j)*256+sx+w].copy(); d=slice((dy+j)*256+dx,(dy+j)*256+dx+len(src))
                if lop&8: cur=s.vram[d]; s.vram[d]=np.where(src==0,cur,src)
                else: s.vram[d]=src
            s.log['lmmm']+=1
    # ---- Geo3D
    def gwrite(s,v):
        i=s.gi; gb=s.gb; s.log['geo_bytes']+=1
        if i==0x50:
            b=s.gbuf[0x50]; b.append(v)
            if len(b)==6: s.gv[gb[0x40]]=struct.unpack('<hhh',bytes(b)); gb[0x40]=(gb[0x40]+1)&255; b.clear()
            return
        if i==0x52:
            b=s.gbuf[0x52]; b.append(v)
            if len(b)==11: s.gf[gb[0x58]]=(tuple(b[:4]),struct.unpack('<hhh',bytes(b[4:10])),b[10]); gb[0x58]=(gb[0x58]+1)&255; b.clear()
            return
        if i==0x53:
            b=s.gbuf[0x53]; b.append(v)
            if len(b)==8: s.gt[gb[0x65]]=tuple(b); gb[0x65]=(gb[0x65]+1)&255; b.clear()
            return
        gb[i]=v
        if i<0x24 or 0x40<=i<=0x4F or 0x58<=i<=0x5F or 0x60<=i<=0x67: s.gi=i+1
        if i==0x48 and v&1: s.run(v)
    def run(s,ctrl):
        s.log['geo_runs']+=1; gb=s.gb; w=lambda o:struct.unpack('<h',bytes(gb[o:o+2]))[0]
        M=[w(2*k)/16384 for k in range(9)]; T=[w(18+2*k) for k in range(3)]
        F,CX,CY,ZN,SW,SH=w(24),w(26),w(28),w(30),w(32),w(34)
        L=[w(0x5A+2*k)/16384 for k in range(3)]
        texx=gb[0x60]|gb[0x61]<<8; texy=gb[0x62]|gb[0x63]<<8; tstride=gb[0x64]
        yp=(gb[0x46]|gb[0x47]<<8); nv=gb[0x42]; nf=gb[0x59]; lop=gb[0x45]&15
        P=[[sum(M[3*i+j]*v[j] for j in range(3))+T[i] for i in range(3)] for v in (s.gv[k] for k in range(nv))]
        LM=[sum(M[3*i+j]*L[i] for i in range(3)) for j in range(3)]
        todo=[]
        for k in range(nf):
            idx,n,base=s.gf[k]; pts=[P[i] for i in idx]
            if any(p[2]<ZN for p in pts): s.log.setdefault('near_skipped',0); s.log['near_skipped']+=1; continue
            sc=[(CX+F*p[0]/p[2],CY-F*p[1]/p[2]) for p in pts]
            area=(sc[1][0]-sc[0][0])*(sc[2][1]-sc[0][1])-(sc[2][0]-sc[0][0])*(sc[1][1]-sc[0][1])
            if area<=0: continue
            lv=min(6,int(7*max(0,sum(LM[i]*n[i]/16384 for i in range(3)))))
            todo.append((sum(p[2] for p in pts),k,lv,sc,base))
        for _,k,lv,sc,base in sorted(todo,key=lambda t:-t[0]):
            s.log['faces']+=1
            if (ctrl&4) and (base&0x80) and k in s.gt: s.raster_tex(sc,s.gt[k],lv,texx,texy,tstride,yp,SW,SH,lop)
    def raster_tex(s,sc,uv,lv,texx,texy,tstride,yp,SW,SH,lop):
        pts=[(sc[i][0],sc[i][1],uv[2*i],uv[2*i+1]) for i in range(4)]
        ys=[p[1] for p in pts]; y0=max(0,int(np.ceil(min(ys)-0.5))); y1=min(SH-1,int(np.floor(max(ys)-0.5)))
        for y in range(y0,y1+1):
            yc=y+0.5; xs=[]
            for a in range(4):
                A=pts[a];B=pts[(a+1)%4]
                if (A[1]<=yc<B[1]) or (B[1]<=yc<A[1]):
                    t=(yc-A[1])/(B[1]-A[1]); xs.append((A[0]+(B[0]-A[0])*t,A[2]+(B[2]-A[2])*t,A[3]+(B[3]-A[3])*t))
            if len(xs)<2: continue
            xs.sort(key=lambda e:e[0]); l,r=xs[0],xs[-1]
            xl=max(0,int(np.ceil(l[0]-0.5))); xr=min(SW-1,int(np.floor(r[0]-0.5)))
            if xr<xl: continue
            X=np.arange(xl,xr+1); den=(r[0]-l[0]) or 1.0; t=(X+0.5-l[0])/den
            u=np.clip(np.floor(l[1]+(r[1]-l[1])*t).astype(int),0,255); v=np.clip(np.floor(l[2]+(r[2]-l[2])*t).astype(int),0,255)
            src=s.vram[((texy+lv*tstride+v)&1023)*256+((texx+u)&255)]
            d=(y+yp)*256+X
            if lop&8: keep=src!=0; s.vram[d[keep]]=src[keep]
            else: s.vram[d]=src
            s.log['tex_pixels']+=len(X)
    def run_frames(s,n,max_chunks=200000):
        for _ in range(max_chunks):
            s.m.ticks_to_stop=100000; s.m.run()
            if len(s.frames)>=n: break
        return s.frames

if __name__=='__main__':
    n=int(sys.argv[1]) if len(sys.argv)>1 else 3
    pre=sys.argv[2] if len(sys.argv)>2 else 'out/harness'
    M=Machine(lambda row:0xFF); fr=M.run_frames(n)
    print('frames',len(fr),M.log)
    for i,f in enumerate(fr): f.resize((768,636),Image.NEAREST).save(f'{pre}_{i}.png')
