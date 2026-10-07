"""Monta a ROM ASCII16: banco 0 = 0x4000..0x7FFF e banco 1 = 0x8000..0xBFFF do .ihx; bancos 2.. = out/data_banks.bin."""
import sys,os
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
    assert lo>=0x4000 and hi<0xC000,(hex(lo),hex(hi))
    b0=bytearray(b'\xff'*16384);b1=bytearray(b'\xff'*16384)
    for a,v in mem.items():
        if a<0x8000:b0[a-0x4000]=v
        else:b1[a-0x8000]=v
    used1=max((a for a in mem if a>=0x8000),default=0x8000)-0x8000+1
    d=open(data,'rb').read(); assert len(d)%16384==0
    rom=bytes(b0)+bytes(b1)+d
    n=16
    while n*16384<len(rom):n*=2
    rom=rom.ljust(n*16384,b'\xff')
    open(out,'wb').write(rom)
    print('ROM %d KiB (%d bancos); código no banco 1: %d de 16384 bytes; dados: %d bancos'%(len(rom)//1024,n,used1,len(d)//16384))
if __name__=='__main__':main(*sys.argv[1:4])
