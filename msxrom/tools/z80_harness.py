"""Executa a ROM (out/msxdoom.rom) num emulador de Z80 (pip z80) com um MODELO SIMPLIFICADO de
V9968 (SCREEN 5, LMMV, paleta, páginas) e Geo3D (registradores de geo3d_engine.v; faces preenchidas).
Validação classe SIMULATED: não é openMSX/FPGA/hardware. Uso: z80_harness.py [frames] [saida_prefixo]"""
import sys,struct,os
from z80 import Z80Machine
from PIL import Image,ImageDraw
ROM=os.path.join(os.path.dirname(__file__),'..','out','msxdoom.rom')

class Machine:
    def __init__(s,keys_fn):
        s.m=Z80Machine(); mem=s.m.memory
        rom=open(ROM,'rb').read(); mem[0x4000:0x8000]=rom
        for a in (0x5F,0x180,0x24): mem[a]=0xC9         # stubs de BIOS (CHGMOD/CHGCPU/ENASLT): RET
        mem[0x2D]=3                                      # turbo R
        s.m.pc=0x4010 if False else rom[2]|rom[3]<<8; s.m.sp=0xF380
        s.vram=Image.new('P',(256,1024),0); s.pal=[(0,0,0)]*16
        s.vreg=[0]*64; s.vlatch=None; s.ind=0; s.pal_i=0; s.pal_lo=None
        s.gi=0; s.gb=bytearray(256); s.gcnt=0; s.verts=[None]*256; s.faces=[None]*256
        s.cmd=[]; s.ppi=0; s.keys_fn=keys_fn; s.frames=[]; s.runs=0; s.faces_drawn=0; s.shown=0
        s.io_log={'geo_runs':0,'lmmv':0,'page_flips':0}
        s.m.set_input_callback(s.inp); s.m.set_output_callback(s.out)
    # ---- entrada
    def inp(s,port):
        port&=0xFF
        if port==0x99: return 0x80                       # CE=0, vblank=1
        if port==0x9D: return 0                          # RUN busy = 0
        if port==0xA9: return s.keys_fn(s.ppi&0x0F)
        return 0xFF
    # ---- saída
    def out(s,port,v):
        port&=0xFF; v&=0xFF
        if port==0x99:
            if s.vlatch is None: s.vlatch=v
            else:
                if v&0x80: s.wreg(v&0x3F,s.vlatch)
                s.vlatch=None
        elif port==0x9A:
            if s.pal_lo is None: s.pal_lo=v
            else:
                r,b,g=(s.pal_lo>>4)&7,s.pal_lo&7,v&7
                s.pal[s.pal_i&15]=(int(r*255/7),int(g*255/7),int(b*255/7)); s.pal_i+=1; s.pal_lo=None
        elif port==0x9B:
            s.vreg[s.ind]=v
            if s.ind==46: s.exec_vdp()
            s.ind+=1
        elif port==0x9D: s.gi=v; s.gcnt=0
        elif port==0x9F: s.gwrite(v)
        elif port==0xAA: s.ppi=v
    def wreg(s,r,v):
        if r==17: s.ind=v&0x3F
        elif r==16: s.pal_i=v&15; s.pal_lo=None
        else:
            s.vreg[r]=v
            if r==2: s.page_flip(v)
    def page_flip(s,v):
        s.io_log['page_flips']+=1; pg=(v>>5)&3
        im=s.vram.crop((0,pg*256,256,pg*256+212)); im.putpalette([c for rgb in s.pal for c in rgb]); s.frames.append(im.convert('RGB'))
    def exec_vdp(s):
        r=s.vreg
        if r[46]&0xF0==0x80:                              # LMMV
            x=r[36]|r[37]<<8; y=r[38]|r[39]<<8; w=r[40]|r[41]<<8; h=r[42]|r[43]<<8
            ImageDraw.Draw(s.vram).rectangle([x,y,x+w-1,y+h-1],fill=r[44]&15); s.io_log['lmmv']+=1
    # ---- Geo3D (mapa 0x00-0x5F do geo3d_engine.v)
    def gwrite(s,v):
        i=s.gi
        if i==0x50:                                       # VDATA: 6 B/vértice em VADDR
            s.gb_v=getattr(s,'gb_v',bytearray())+bytes([v])
            if len(s.gb_v)==6: s.verts[s.gb[0x40]]=struct.unpack('<hhh',s.gb_v); s.gb[0x40]=(s.gb[0x40]+1)&255; s.gb_v=bytearray()
            return
        if i==0x52:                                       # FDATA: 11 B/face em FADDR
            s.gb_f=getattr(s,'gb_f',bytearray())+bytes([v])
            if len(s.gb_f)==11: s.faces[s.gb[0x58]]=(tuple(s.gb_f[:4]),struct.unpack('<hhh',bytes(s.gb_f[4:10])),s.gb_f[10]); s.gb[0x58]=(s.gb[0x58]+1)&255; s.gb_f=bytearray()
            return
        s.gb[i]=v
        if v is not None and (i<0x24 or 0x40<=i<=0x4F or 0x58<=i<=0x5F or 0x60<=i<=0x67):
            s.gi=i+1
        if i==0x48 and v&1: s.run(v)
    def run(s,ctrl):
        s.io_log['geo_runs']+=1
        gb=s.gb; w=lambda o:struct.unpack('<h',bytes(gb[o:o+2]))[0]
        M=[w(2*k)/16384 for k in range(9)]; T=[w(18+2*k) for k in range(3)]
        F,CX,CY,ZN=w(24),w(26),w(28),w(30); L=[w(0x5A+2*k)/16384 for k in range(3)]
        yp=gb[0x46]|gb[0x47]<<8; nv=gb[0x42]; nf=gb[0x59]
        P=[[sum(M[3*i+j]*v[j] for j in range(3))+T[i] for i in range(3)] for v in (s.verts[k] for k in range(nv))]
        LM=[sum(M[3*i+j]*L[i] for i in range(3)) for j in range(3)]
        todo=[]
        for k in range(nf):
            idx,n,base=s.faces[k]; pts=[P[i] for i in idx]
            if any(p[2]<ZN for p in pts): continue
            sc=[(CX+F*p[0]/p[2],CY-F*p[1]/p[2]) for p in pts]
            area=(sc[1][0]-sc[0][0])*(sc[2][1]-sc[0][1])-(sc[2][0]-sc[0][0])*(sc[1][1]-sc[0][1])
            if area<=0: continue
            lv=min(6,int(7*max(0,sum(LM[i]*n[i]/16384 for i in range(3))))); todo.append((sum(p[2] for p in pts),(base+lv)&15,sc))
        d=ImageDraw.Draw(s.vram)
        for _,c,sc in sorted(todo,key=lambda t:-t[0]): d.polygon([(x,y+yp) for x,y in sc],fill=c)
        s.faces_drawn+=len(todo); s.last_runs=getattr(s,'last_runs',[])[-12:]+[(nv,nf,tuple(T),len(todo))]
    def run_frames(s,n,max_chunks=4000):
        for _ in range(max_chunks):
            s.m.ticks_to_stop=20000; s.m.run()
            if len(s.frames)>=n: break
        return s.frames

if __name__=='__main__':
    n=int(sys.argv[1]) if len(sys.argv)>1 else 4
    pre=sys.argv[2] if len(sys.argv)>2 else 'out/harness'
    script=lambda row:0xFF
    M=Machine(script); fr=M.run_frames(n)
    print('frames',len(fr),M.io_log,'faces desenhadas',M.faces_drawn)
    for i,f in enumerate(fr): f.resize((768,636),Image.NEAREST).save(f'{pre}_{i}.png')
