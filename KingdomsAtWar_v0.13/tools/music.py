#!/usr/bin/env python3
"""Musica e efeitos em FM (MSX-MUSIC, OPLL YM2413) para o jogo.
As musicas sao escritas em uma notacao MML simples (e por progressoes de acordes) e compiladas em FLUXOS de escritas nos
registradores do OPLL, um fluxo por musica, lidos direto da ROM pela interrupcao do jogo (src/music.h).

Fluxo (bytes):  0x00..0x3F registrador (o proximo byte e o valor); 0x80+k espera k+1 quadros (k=0..124);
                0xFD proximo banco de ROM; 0xFE volta ao ponto de repeticao da musica; 0xFF fim.
Canais do OPLL: melodicos 0..4 = musica, 5 = efeitos sonoros, 6..8 = bateria (modo ritmico).
Notacao MML: a-g (+ - sustenido/bemol), r pausa, duracao em numero (4 = semínima, 8 = colcheia, ., &), o<n> oitava, < >, l<n>, [ ... ]n repete.
Bateria: b bumbo, s caixa, t tom, c prato, h chimbal, B bumbo+chimbal, S caixa+chimbal.
Gera src/music_data.h e os bancos src/banks/art_s<N>_b3.c a partir de FIRST_BANK."""
import math,re,sys
from pathlib import Path
r=Path(__file__).resolve().parents[1]
FIRST_BANK=63
BANK_SIZE=8192
UNITS=192            # unidades por semibreve
def fnum_block(midi):
    f=440.0*2**((midi-69)/12.0)
    for block in range(0,8):
        fn=round(f*2**(19-block)/49716.0)
        if fn<=511:return fn,block
    raise ValueError(midi)
NOTE={'c':0,'d':2,'e':4,'f':5,'g':7,'a':9,'b':11}
DRUM={'b':0x10,'s':0x08,'t':0x04,'c':0x02,'h':0x01,'B':0x11,'S':0x09}
def expand(s):
    pat=re.compile(r'\[([^\[\]]*)\](\d*)')
    while True:
        m=pat.search(s)
        if not m:return s
        s=s[:m.start()]+(' '+m.group(1)+' ')*int(m.group(2) or 2)+s[m.end():]
def parse(mml,drums=False):
    """devolve eventos (inicio, duracao, valor) em unidades; valor = midi ou mascara da bateria"""
    ev=[];t=0;o=4;l=4;mml=expand(mml.replace('|',' '))
    toks=re.findall(r'[a-hrstA-GS][+\-#]?\d*\.*&?|[<>]|o\d|l\d+\.*|\S',mml)
    tie=None
    def dur(sfx,default):
        m=re.match(r'(\d*)(\.*)',sfx);n=int(m.group(1)) if m.group(1) else default
        u=UNITS//n;add=u
        for _ in m.group(2):add//=2;u+=add
        return u
    for tk in toks:
        if tk=='<':o-=1;continue
        if tk=='>':o+=1;continue
        if tk[0]=='o':o=int(tk[1:]);continue
        if tk[0]=='l' and tk[1:2].isdigit():
            m=re.match(r'l(\d+)(\.*)',tk);l=int(m.group(1));
            if m.group(2):raise ValueError('l com ponto nao suportado')
            continue
        if tk[0]=='r':t+=dur(tk[1:],l);tie=None;continue
        c=tk[0];rest=tk[1:]
        if drums:
            if c not in DRUM:raise ValueError(tk)
            u=dur(rest.rstrip('&'),l);ev.append((t,u,DRUM[c]));t+=u;continue
        if c not in NOTE:raise ValueError(tk)
        acc=0
        while rest[:1] in ('+','#','-') and rest:
            acc+=1 if rest[0] in '+#' else -1;rest=rest[1:]
        tied=rest.endswith('&');rest=rest.rstrip('&')
        u=dur(rest,l);midi=12*(o+1)+NOTE[c]+acc
        if tie and tie[2]==midi and tie[0]+tie[1]==t:
            ev[-1]=(tie[0],tie[1]+u,midi);tie=(tie[0],tie[1]+u,midi) if tied else None
        else:
            ev.append((t,u,midi));tie=(t,u,midi) if tied else None
        t+=u
    return ev,t
CH_INST={}
class Song:
    def __init__(self,name,bpm,loop=True,fps=60):
        self.name=name;self.bpm=bpm;self.loop=loop;self.tracks=[];self.drums=None;self.drum_vol=(8,8,8,8,8)
        self.tpw=14400.0/bpm*fps/60.0
    def melodic(self,ch,inst,vol,events,gate=0.85):
        self.tracks.append(dict(ch=ch,inst=inst,vol=vol,ev=events,gate=gate))
    def mml(self,ch,inst,vol,text,gate=0.85):
        ev,t=parse(text);self.melodic(ch,inst,vol,ev,gate);return t
    def drum(self,text,vol=(8,8,8,8,8)):
        self.drums,t=parse(text,True);self.drum_vol=vol;return t
    def tick(self,u):return int(round(u*self.tpw/UNITS))
def compile_song(sg,total_units=None):
    """eventos por quadro: {tick: [(reg,val)...]}"""
    W={}
    def put(t,reg,val):W.setdefault(t,[]).append((reg,val))
    state={}
    # prelude: modo ritmico e silencio
    pre=[]
    for ch in range(5):pre.append((0x20+ch,0))
    if sg.drums:
        pre+= [(0x16,0x20),(0x26,0x05),(0x17,0x50),(0x27,0x05),(0x18,0xC0),(0x28,0x01),(0x0E,0x20)]
        bd,hh,sd,tom,tc=sg.drum_vol
        pre+=[(0x36,bd&15),(0x37,(hh<<4)|sd),(0x38,(tom<<4)|tc)]
    else:pre.append((0x0E,0x00))
    for reg,val in pre:put(0,reg,val)
    end_tick=0
    for tr in sg.tracks:
        ch=tr['ch'];evs=sorted(tr['ev'])
        for i,(s_,d,m) in enumerate(evs):
            ts=sg.tick(s_);te=ts+max(1,int(round(d*tr['gate']*sg.tpw/UNITS)))
            if i+1<len(evs):te=min(te,max(ts+1,sg.tick(evs[i+1][0])-1))
            fn,bl=fnum_block(m)
            key=(tr['inst'],tr['vol'])
            if state.get(ch)!=key:put(ts,0x30+ch,(tr['inst']<<4)|(15-tr['vol']));state[ch]=key
            put(ts,0x10+ch,fn&255);put(ts,0x20+ch,0x10|(bl<<1)|(fn>>8))
            put(te,0x20+ch,(bl<<1)|(fn>>8)) if te!=ts else None
            end_tick=max(end_tick,te)
    if sg.drums:
        last=None
        for s,d,mask in sorted(sg.drums):
            ts=sg.tick(s);put(ts,0x0E,0x20|mask);put(ts+1,0x0E,0x20);end_tick=max(end_tick,ts+1)
    # remover escritas que fiquem sobrepostas: no mesmo quadro, key-off antes de key-on do mesmo canal
    return W,end_tick
def order_frame(items):
    """key-off (reg 0x2x sem bit 4) primeiro; depois volume/instrumento; depois frequencia; depois key-on; rhythm por ultimo."""
    def k(it):
        reg,val=it
        if 0x20<=reg<=0x28 and not val&0x10:return 0
        if 0x30<=reg<=0x38:return 1
        if 0x10<=reg<=0x18:return 2
        if 0x20<=reg<=0x28:return 3
        return 4 if reg==0x0E else 1
    # mantem ordem de insercao dentro do mesmo grupo; para o 0x0E duas escritas no mesmo quadro: so a ultima vale
    out=sorted(items,key=k)
    seen={}
    for it in out:
        if it[0]==0x0E:seen[0x0E]=it
    res=[it for it in out if it[0]!=0x0E];
    if 0x0E in seen:res.append(seen[0x0E])
    return res
def stream_bytes(W,length):
    toks=[];prev=0
    def waits(n):
        while n>0:
            c=min(n,125);toks.append(bytes([0x80+c-1]));n-=c
    for tk in sorted(W):
        if tk>=length:continue
        waits(tk-prev)
        for reg,val in order_frame(W[tk]):toks.append(bytes([reg,val]))
        prev=tk
    waits(length-prev)
    return toks
def pack_tokens(toks,loop_index):
    """junta tokens em bancos de 8 KB (0xFD entre eles); devolve (bytes por banco, banco/offset do ponto de repeticao)"""
    banks=[bytearray()];loop_pos=(0,0)
    for i,tk in enumerate(toks):
        if len(banks[-1])+len(tk)>BANK_SIZE-1:banks[-1].append(0xFD);banks.append(bytearray())
        if i==loop_index:loop_pos=(len(banks)-1,len(banks[-1]))
        banks[-1]+=tk
    return banks,loop_pos

# ----------------------------------------------------------------------------------------------------------------------
# harmonia
PC={'C':0,'D':2,'E':4,'F':5,'G':7,'A':9,'B':11}
def chord(name):
    """'Dm','Bb','A','Gm','E7','Am' -> (raiz pitch class, [intervalos])"""
    m=re.match(r'([A-G])([b#]?)(m?)(7?)',name);root=PC[m.group(1)]+(1 if m.group(2)=='#' else -1 if m.group(2)=='b' else 0)
    iv=[0,3 if m.group(3) else 4,7]+([10] if m.group(4) else [])
    return root%12,iv
def bar_units(beats=4):return UNITS//4*beats
def accompaniment(sg,bars,bass_ch,arp_ch,pad_ch,bass_inst=14,arp_inst=2,pad_inst=8,bass_vol=11,arp_vol=8,pad_vol=6,bass_oct=2,arp_oct=3,pattern='march'):
    bass=[];arp=[];pad=[];t=0;B=bar_units()
    for name in bars:
        root,iv=chord(name);rb=12*(bass_oct+1)+root;ra=12*(arp_oct+1)+root
        tones=[ra+i for i in iv]
        if pattern=='march':        # raiz, raiz, quinta, raiz, quinta (4/4)
            seq=[(0,48),(0,24),(7,24),(0,48),(7,48)]
        elif pattern=='drive':      # colcheias de raiz com oitava
            seq=[(0,24),(0,24),(12,24),(0,24),(0,24),(0,24),(12,24),(7,24)]
        elif pattern=='slow':       # raiz longa e quinta
            seq=[(0,96),(7,96)]
        else:seq=[(0,B)]
        tt=t
        for off,u in seq:bass.append((tt,u,rb+off));tt+=u
        # arpejo em colcheias subindo e descendo
        order=[0,1,2,1] if len(tones)==3 else [0,1,2,3,2,1]
        n=B//24;tt=t
        for k in range(n):
            bass_n=tones[order[k%len(order)]];arp.append((tt,24,bass_n));tt+=24
        # pad: terca e quinta sustentadas (uma voz so: a terca)
        pad.append((t,B,rb+12+iv[1]));
        t+=B
    sg.melodic(bass_ch,bass_inst,bass_vol,bass,0.8);sg.melodic(arp_ch,arp_inst,arp_vol,arp,0.55);sg.melodic(pad_ch,pad_inst,pad_vol,pad,0.97)
    return t
def drums_pattern(bars,kind):
    out=[]
    for b in range(bars):
        if kind=='epic':out.append('B8 r8 r8 r8 s8 r8 h8 r8' if b%2 else 'B8 r8 h8 r8 S8 r8 h8 r8')
        elif kind=='battle':out.append('B8 h8 S8 h8 B8 h8 S8 t8' if b%4!=3 else 'B8 h8 S8 h8 t8 t8 S8 s8')
        elif kind=='calm':out.append('b4 r4 r4 r4' if b%2==0 else 'r4 r4 h4 r4')
        else:out.append('r1')
    return ' '.join(out)
def melody(bars):return ' '.join(bars)

SONGS=[]
# ---------------------------------------------------------------- 0: titulo / menu (re menor)
def song_title():
    s=Song('title',78)
    ch=['Dm','Dm','Bb','F','C','C','Gm','A','Dm','Dm','Bb','F','Gm','A','Dm','Dm']
    total=accompaniment(s,ch,2,3,4,pattern='march',arp_inst=2)
    mel=['o4 d2 f4 a4','o5 d2. c4','o4 b-2 o5 d4 f4','o5 f2 e4 d4','o5 c2 e4 g4','o5 e2. d4','o4 b-2 o5 d4 g4','o5 c+2 e2',
         'o5 d2 f4 a4','o5 a2. g4','o5 f2 d4 f4','o5 a2 g4 f4','o5 d2 g4 b-4','o5 a2 g4 e4','o5 f2 e4 d4','o5 d1']
    s.mml(0,7,12,melody(mel),0.9)
    low=['r1']*8+['o4 d2 f4 a4','o4 a2. g4','o4 f2 d4 f4','o4 a2 g4 f4','o4 d2 g4 b-4','o4 a2 g4 e4','o4 f2 e4 d4','o4 d1']
    s.mml(1,9,8,melody(low),0.9)
    s.drum(drums_pattern(16,'epic'),(6,10,7,8,10))
    s.total=total;return s
# ---------------------------------------------------------------- 1: campanha tranquila (la menor)
def song_calm():
    s=Song('calm',84)
    ch=['Am','F','C','G','Am','F','Dm','E','Am','F','C','G','F','C','Dm','E']
    total=accompaniment(s,ch,2,3,4,pattern='slow',arp_inst=3,arp_vol=7,bass_vol=9,pad_inst=4,pad_vol=5,arp_oct=3)
    mel=['o5 e2 c4 d4','o5 c2 a4 c4','o5 e2 g4 e4','o5 d2. r4','o5 c2 e4 a4','o5 a2 f4 a4','o5 f2 d4 f4','o5 g+2. r4',
         'o5 e4 a4 e4 c4','o5 c4 a4 f4 a4','o5 e4 g4 c4 e4','o5 d4 g4 b2','o5 a2 f4 c4','o5 g2 e4 c4','o5 f2 d4 e4','o5 e2. r4']
    s.mml(0,4,11,melody(mel),0.92)
    s.drum(drums_pattern(16,'calm'),(5,8,6,6,6))
    s.total=total;return s
# ---------------------------------------------------------------- 2: batalha (re menor, rapido)
def song_battle():
    s=Song('battle',152)
    ch=['Dm','Dm','Bb','C','Dm','Dm','Gm','A','Dm','Bb','C','Am','Bb','Gm','A','A']
    total=accompaniment(s,ch,2,3,4,pattern='drive',arp_inst=15,arp_vol=7,bass_vol=12,pad_inst=9,pad_vol=7)
    mel=['o5 d4 d8 d8 f4 e4','o5 d4 d8 d8 a2','o5 d4 d8 d8 f4 e4','o5 e4 e8 e8 g2',
         'o5 f4 f8 f8 a4 g4','o5 f4 e8 d8 d2','o5 d4 g4 b-4 g4','o5 c+4 e4 a2',
         'o5 a4 a8 a8 f4 d4','o5 f4 d8 f8 b-2','o5 g4 e8 g8 c2','o5 e4 a4 c4 e4',
         'o5 d4 f4 b-4 d4','o5 b-4 g4 d4 g4','o5 e4 a4 c+4 e4','o5 a2 e2']
    s.mml(0,7,13,melody(mel),0.85)
    s.mml(1,6,8,' '.join(['r1']*4+['o4 d2 f2','o4 e2 g2','o4 f2 a2','o4 f2 a2']+['r1']*8),0.95)
    s.drum(drums_pattern(16,'battle'),(5,9,5,6,10))
    s.total=total;return s
# ---------------------------------------------------------------- 3: vitoria (re maior, curta)
def song_victory():
    s=Song('victory',120,loop=False)
    s.mml(0,7,13,'o4 d8 d8 d8 d4. f+8 a2 o5 d2.. r8',0.9)
    s.mml(1,9,9,'o4 a8 a8 a8 a4. a8 f+2 a2.. r8',0.9)
    s.mml(2,14,12,'o3 d4 d4 a4 d4 d2 d2..',0.9)
    s.drum('B8 r8 S8 r8 B8 S8 t8 t8 B2 c2',(6,10,7,8,12))
    s.total=0;return s
# ---------------------------------------------------------------- 4: derrota (re menor, lenta)
def song_defeat():
    s=Song('defeat',58,loop=False)
    s.mml(0,6,12,'o4 a2 g4 f4 e2 d2 c+1',0.97)
    s.mml(1,9,8,'o4 f2 e4 d4 c2 b-2 a1',0.97)
    s.mml(2,14,11,'o2 d1 c1 b-1 a1',0.97)
    s.drum('b4 r4 r4 r4 r1 b4 r4 r4 r4 r1',(8,10,7,8,12))
    s.total=0;return s
# ---------------------------------------------------------------- 5: fim da campanha (re maior, longa)
def song_ending():
    s=Song('ending',100,loop=False)
    ch=['D','G','D','A','Bm','G','A','D']
    accompaniment(s,ch,2,3,4,pattern='march',arp_inst=11,arp_vol=8,bass_vol=11,pad_inst=8,pad_vol=6)
    mel=['o5 d2. f+4','o5 g2 b4 g4','o5 f+2. d4','o5 e2 c+4 e4','o5 d4 f+4 b4 d4','o5 d4 g4 b2','o5 e4 c+4 a4 e4','o5 d1']
    s.mml(0,7,13,melody(mel),0.92)
    s.mml(1,9,8,' '.join(['r1']*4+['o4 b2 a2','o4 b2 g2','o4 g2 e2','o4 f+1']),0.95)
    s.drum('B8 r8 h8 r8 S8 r8 h8 r8 '*7+'B2 c2',(6,10,7,8,12))
    s.total=0;return s
SONG_BUILDERS=[('title',song_title),('calm',song_calm),('battle',song_battle),('victory',song_victory),('defeat',song_defeat),('ending',song_ending)]

# ---------------------------------------------------------------- efeitos sonoros (canal 5)
# ('n',midi,ticks) nota; ('s',m0,m1,ticks) deslizamento; ('r',ticks) pausa
SFX=[
 ('click',   dict(prio=1,inst=12,vol=13,seq=[('n',88,2)])),
 ('confirm',   dict(prio=2,inst=12,vol=14,seq=[('n',76,3),('n',83,5)])),
 ('hit',   dict(prio=2,inst=13,vol=15,seq=[('s',64,48,5)])),
 ('arrow',   dict(prio=1,inst=6,vol=11,seq=[('s',60,84,6)])),
 ('build',   dict(prio=3,inst=14,vol=14,seq=[('n',48,3),('r',3),('n',48,3),('r',3),('n',55,5)])),
 ('done',   dict(prio=4,inst=7,vol=14,seq=[('n',67,5),('n',72,5),('n',76,5),('n',79,12)])),
 ('trained',   dict(prio=4,inst=7,vol=13,seq=[('n',72,5),('n',79,12)])),
 ('gold',   dict(prio=1,inst=12,vol=15,seq=[('n',91,2),('n',96,5)])),
 ('wood',   dict(prio=1,inst=14,vol=15,seq=[('s',52,40,4)])),
 ('alarm',   dict(prio=5,inst=9,vol=15,seq=[('n',50,12),('r',3),('n',50,12),('r',3),('n',45,20)])),
 ('death',   dict(prio=3,inst=1,vol=13,seq=[('s',60,36,18)])),
 ('error',   dict(prio=3,inst=13,vol=13,seq=[('n',40,4),('r',2),('n',38,6)])),
]
def compile_sfx(spec):
    ch=5;W={};t=0
    def put(tt,reg,val):W.setdefault(tt,[]).append((reg,val))
    put(0,0x20+ch,0);put(0,0x30+ch,(spec['inst']<<4)|(15-spec['vol']))
    last_block=0
    for it in spec['seq']:
        if it[0]=='n':
            fn,bl=fnum_block(it[1]);put(t,0x10+ch,fn&255);put(t,0x20+ch,0x10|(bl<<1)|(fn>>8))
            put(t+it[2]-1,0x20+ch,(bl<<1)|(fn>>8));t+=it[2]
        elif it[0]=='s':
            n=it[3]
            for k in range(n):
                m=it[1]+(it[2]-it[1])*k/(n-1 if n>1 else 1);f=440.0*2**((m-69)/12.0)
                for block in range(0,8):
                    fn=round(f*2**(19-block)/49716.0)
                    if fn<=511:break
                put(t+k,0x10+ch,fn&255);put(t+k,0x20+ch,0x10|(block<<1)|(fn>>8))
            put(t+n-1,0x20+ch,(block<<1)|(fn>>8));t+=n
        else:t+=it[1]
    put(t,0x20+ch,0)
    toks=[];prev=0
    for tk in sorted(W):
        gap=tk-prev
        while gap>0:
            c=min(gap,125);toks.append(bytes([0x80+c-1]));gap-=c
        for reg,val in order_frame(W[tk]):toks.append(bytes([reg,val]))
        prev=tk
    toks.append(bytes([0xFF]))
    return b''.join(toks)
def main():
    banks_all=[];song_info=[]
    for name,fn in SONG_BUILDERS:
        sg=fn();W,end=compile_song(sg)
        bar=sg.tick(bar_units())
        length=((end+bar)//bar)*bar if sg.loop else end+1
        toks=stream_bytes(W,length)
        toks.append(bytes([0xFE]) if sg.loop else bytes([0xFF]))
        banks,loop=pack_tokens(toks,0)
        song_info.append((name,len(banks_all),len(banks),loop,sum(len(b) for b in banks),length/60.0))
        banks_all+= [bytes(b) for b in banks]
    sfx_bank=len(banks_all);sfx_blob=bytearray();sfx_ptr=[]
    for name,spec in SFX:
        sfx_ptr.append(len(sfx_blob));sfx_blob+=compile_sfx(spec)
    assert len(sfx_blob)<BANK_SIZE
    banks_all.append(bytes(sfx_blob))
    for i,b in enumerate(banks_all):
        (r/'src/banks'/f'art_s{FIRST_BANK+i}_b3.c').write_text('#include "msxgl.h"\nconst u8 artbank_%d[] = {\n'%(FIRST_BANK+i)+'\n'.join(','.join(map(str,b[j:j+32]))+',' for j in range(0,len(b),32))+'\n};\n')
    ids='/* gerado por tools/music.py */\n#define SONG_COUNT %d\n#define SFX_COUNT %d\n'%(len(song_info),len(SFX))
    for k,(name,*_) in enumerate(song_info):ids+='#define SONG_%s %d\n'%(name.upper(),k)
    for k,(name,_) in enumerate(SFX):ids+='#define SFX_%s %d\n'%(name.upper(),k)
    (r/'src/music_ids.h').write_text(ids)
    out='/* gerado por tools/music.py */\n#include "music_ids.h"\n#define SFX_BANK %d\n'%(FIRST_BANK+sfx_bank)
    out+='static const u8 song_bank[]={%s};\n'%','.join(str(FIRST_BANK+s[1]) for s in song_info)
    out+='static const u8 song_loop_bank[]={%s};\n'%','.join(str(FIRST_BANK+s[1]+s[3][0]) for s in song_info)
    out+='static const u16 song_loop_ptr[]={%s};\n'%','.join(str(0xA000+s[3][1]) for s in song_info)
    out+='static const u8 sfx_prio[]={%s};\n'%','.join(str(sp['prio']) for _,sp in SFX)
    out+='static const u16 sfx_ptr[]={%s};\n'%','.join(str(0xA000+p) for p in sfx_ptr)
    (r/'src/music_data.h').write_text(out)
    for name,first,n,loop,size,secs in song_info:print(f'{name:8s} bancos {FIRST_BANK+first}..{FIRST_BANK+first+n-1}  {size} B  {secs:.1f} s')
    print('sfx',len(sfx_blob),'B no banco',FIRST_BANK+sfx_bank,[len(compile_sfx(sp)) for _,sp in SFX])
if __name__=='__main__':main()
