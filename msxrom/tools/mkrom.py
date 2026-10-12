"""Monta a ROM ASCII16: banco 0 = 0x4000..0x7FFF e banco 1 = 0x8000..0xBFFF do .ihx; bancos 2.. = out/data_banks.bin."""
import sys,re
from pathlib import Path
def read_ihx(path):
    mem={}
    for line in open(path):
        line=line.strip()
        if not line.startswith(':'):continue
        n=int(line[1:3],16);a=int(line[3:7],16);t=int(line[7:9],16)
        if t==0:
            for i in range(n):mem[a+i]=int(line[9+2*i:11+2*i],16)
    return mem
def main(ihx,data,out):
    mem=read_ihx(ihx)
    lo=min(mem);hi=max(mem)
    assert lo>=0x4000 and hi<0xF000 and all(a<0xC000 or a>=0xD200 for a in mem),(hex(lo),hex(hi))
    linker_map=Path(ihx).with_suffix('.map').read_text()
    for name,start,size in re.findall(r'^(_DATA|_INITIALIZED)\s+([0-9A-F]+)\s+([0-9A-F]+)',linker_map,re.M):
        assert int(start,16)+int(size,16)<=0xD200, ('RAM/code overlap',name,start,size)
    b0=bytearray(b'\xff'*16384);b1=bytearray(b'\xff'*16384)
    for a,v in mem.items():
        if a<0x8000:b0[a-0x4000]=v
        elif a<0xC000:b1[a-0x8000]=v
        else:b0[0x2000+a-0xD200]=v
    assert max((a for a in mem if a<0x8000),default=0x4000)<0x6000
    used1=max((a for a in mem if 0x8000<=a<0xC000),default=0x8000)-0x8000+1
    d=open(data,'rb').read(); assert len(d)%16384==0
    rom=bytes(b0)+bytes(b1)+d
    n=16
    while n*16384<len(rom):n*=2
    rom=rom.ljust(n*16384,b'\xff')
    open(out,'wb').write(rom)
    print('ROM %d KiB (%d bancos); código no banco 1: %d de 16384 bytes; dados: %d bancos'%(len(rom)//1024,n,used1,len(d)//16384))
if __name__=='__main__':main(*sys.argv[1:4])
