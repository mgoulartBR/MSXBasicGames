import os
import struct
d=open(os.environ.get('SP_ROM','rom.md'),'rb').read()
L=lambda a:struct.unpack('>I',d[a:a+4])[0]
W=lambda a:struct.unpack('>H',d[a:a+2])[0]

def dec_mask(p, ntiles, fillmode):
    out=bytearray()
    for _ in range(ntiles):
        if fillmode: fill=d[p]; p+=1
        else: fill=0
        m=int.from_bytes(d[p:p+4],'big'); p+=4
        for b in range(31,-1,-1):
            if (m>>b)&1: out.append(fill)
            else: out.append(d[p]); p+=1
    return bytes(out),p

def dec_lzss(p, nbytes):
    # Okumura LZSS: N=1024 ring, init pos 0x3be, flag bit LSB-first, 1=literal
    ring=bytearray(1024); r=0x3be; out=bytearray()
    while len(out)<nbytes:
        flags=d[p]; p+=1
        for _ in range(8):
            if len(out)>=nbytes: break
            if flags&1:
                c=d[p]; p+=1; out.append(c); ring[r]=c; r=(r+1)&1023
            else:
                b1=d[p]; b2=d[p+1]; p+=2
                w=(b1<<8)|b2; pos=w&0x3ff; ln=(w>>10)+3   # per $24F9C
                for k in range(ln):
                    c=ring[(pos+k)&1023]; out.append(c); ring[r]=c; r=(r+1)&1023
                    if len(out)>=nbytes: break
            flags>>=1
    return bytes(out),p

def load_block(t):
    """returns (dest, data_bytes) for a tile block at address t"""
    ty=W(t); dest=W(t+2); size=W(t+4)  # size in words
    nb=size*2
    if ty==0: return dest, d[t+6:t+6+nb]
    nt=nb//32
    if ty==2: data,_=dec_mask(t+6,nt,True)
    elif ty==3: data,_=dec_mask(t+6,nt,False)
    elif ty==4: data,_=dec_lzss(t+6,nb)
    else: return dest,None
    return dest,data
def pal(addr):
    off=d[addr]; cnt=d[addr+1]+1
    return off,[W(addr+2+2*i) for i in range(cnt)]
def rgb(c):  # MD 0BGR, 3 bits/channel
    return (((c>>1)&7)*36,((c>>5)&7)*36,((c>>9)&7)*36)
