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
# modelo de referência do Geo3D (repo do desenvolvedor); caminho configurável por GEO3D_SIM
_SIM=os.environ.get('GEO3D_SIM','/home/user/alexmoncks/v9968_cartridge/geo3d/sim')
sys.path.insert(0,_SIM)
from gen_scenes import render_faces as _render_faces

class Machine:
    def __init__(s,keys_fn,rom=ROM,geo=True):
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
        s.geo=geo; s.keys_fn=keys_fn; s.ppi=0; s.frames=[]
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
        if port==0x9D: return 0 if s.geo else 0xFF                    # sem Geo3D: barramento flutuante
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
        elif cmd==3:                                                 # LRMM (R#32..45, R#47..50, comando em R#46)
            s.lrmm(list(r[32:46])+list(r[47:51])+[r[46]]); s.log['lrmm_cpu']=s.log.get('lrmm_cpu',0)+1
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
        """RUN do Geo3D executado pelo MODELO DE REFERENCIA do desenvolvedor (alexmoncks/V9968_Cartridge geo3d/sim/gen_scenes.py,
        bit-exato com o RTL) + executor de LRMM conforme vdp_command.v. Cai no meu rasterizador aproximado se o modelo faltar."""
        s.log['geo_runs']+=1; gb=s.gb; w=lambda o:struct.unpack('<h',bytes(gb[o:o+2]))[0]
        cfg=[w(2*k) for k in range(9)]+[w(18+2*k) for k in range(3)]+[w(24+2*k) for k in range(6)]
        nv=gb[0x42]; nf=gb[0x59]; light=[w(0x5A+2*k) for k in range(3)]
        verts=[s.gv[k] for k in range(nv)]
        faces=[tuple(s.gf[k][0])+tuple(s.gf[k][1])+(s.gf[k][2],) for k in range(nf)]
        tex=dict(on=bool(ctrl&4),texx=gb[0x60]|gb[0x61]<<8,texy=gb[0x62]|gb[0x63]<<8,tstride=gb[0x64],uv=[s.gt.get(k,(0,)*8) for k in range(nf)])
        cmds,skip,draw,cull=_render_faces(cfg,verts,faces,gb[0x45]&15,gb[0x46]|gb[0x47]<<8,light,tex)
        s.log['faces']+=draw; s.log['near_skipped']=s.log.get('near_skipped',0)+skip
        for c in cmds:
            if len(c)==19: s.lrmm(c)
            else: s.line(c)
    def line(s,c):
        """LINE horizontal (vdp_command.v): NX+1 pixels de cor CLR a partir de (DX,DY); LOP bit3 = transparente quando a cor e 0."""
        x=c[0]|(c[1]&1)<<8; y=(c[2]|c[3]<<8)&0x3FF; nx=c[4]|(c[5]&7)<<8; col=c[8]; lop=c[10]&15
        if lop&8 and col==0: return
        x1=min(255,x+nx); s.vram[y*256+x:y*256+x1+1]=col; s.log['line_px']=s.log.get('line_px',0)+(x1-x+1)
    def lrmm(s,c):
        """LRMM (vdp_command.v): texel de origem = (SX<<8 + i*VX, SY<<8 + i*VY) >> 8 na linha; a cada linha seguinte (NY>1) a origem avanca (-VY,+VX).
        Janela de origem R#51..58: fora dela vale a cor; o endereco da VRAM faz wrap (X modulo 256, Y modulo 1024). TIMP via func_lop."""
        sx=c[0]|(c[1]&15)<<8; sy=c[2]|(c[3]&31)<<8; dx=c[4]|(c[5]&1)<<8; dy=(c[6]|c[7]<<8)&0x3FF
        nx=c[8]|(c[9]&7)<<8; ny=max(1,c[10]|(c[11]&7)<<8); col=c[12]; lop=c[18]&15
        du=c[14]|c[15]<<8; dv=c[16]|c[17]<<8
        du-=65536*(du>>15); dv-=65536*(dv>>15)
        wex=s.vreg[55]|(s.vreg[56]&1)<<8; wey=s.vreg[57]|(s.vreg[58]&7)<<8
        for j in range(ny):
            for i in range(nx):
                x=((sx<<8)-j*dv+i*du)>>8; y=((sy<<8)+j*du+i*dv)>>8
                d=dx+i
                if d>255: break
                src=int(s.vram[(y&1023)*256+(x&255)]) if (0<=x<=wex and 0<=y<=wey) else col
                if lop&8 and src==0: continue
                s.vram[((dy+j)&1023)*256+d]=src
        s.log['tex_pixels']+=nx*ny
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
