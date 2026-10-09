"""Testes de logica do E1M1 no Z80 emulado (SIMULATED). Precisa do WAD: E1M1_WAD=/caminho/doom1.wad e de private/e1m1.rom (build_e1m1.sh)."""
import re,sys,os,math
sys.path.insert(0,os.path.dirname(__file__))
from e1m1_sim import make,symbols
import e1m1_pvs as P, e1m1_map
L=P.load(os.environ['E1M1_WAD']); R=L.R
KEYS={'left':0x10,'right':0x80,'up':0x20,'down':0x40,'fire':0x01}
held=set()
def kb(row):
    v=0xFF
    if row==8:
        for k in held:
            if k in KEYS: v&=~KEYS[k]
    if row==7 and 'use' in held: v&=~0x80
    return v
M=make(kb); sym=symbols(); mem=M.m.memory
def s16(v): return v-65536 if v>32767 else v
def rd8(n): return mem[sym[n]]
def rd16(n): return s16(mem[sym[n]]|mem[sym[n]+1]<<8)
def wr16(n,v): mem[sym[n]:sym[n]+2]=(v&0xFFFF).to_bytes(2,'little')
def wr8(n,v): mem[sym[n]]=v&255
TS=11
def th(i):
    o=sym['th']+i*TS; return dict(x=s16(mem[o]|mem[o+1]<<8),y=s16(mem[o+2]|mem[o+3]<<8),hp=s16(mem[o+4]|mem[o+5]<<8),kind=mem[o+6],st=mem[o+7],t=mem[o+8],on=mem[o+9])
def set_th(i,**kw):
    o=sym['th']+i*TS
    for k,v in kw.items():
        off={'x':0,'y':2,'hp':4}[k]; mem[o+off:o+off+2]=(v&0xFFFF).to_bytes(2,'little')
def frames(n): t=len(M.frames); M.run_frames(t+n)
def free(x,y): 
    cx,cy=int((x-R.x0)//8),int((y-R.y0)//8); return 0<=cx<R.nx and 0<=cy<R.ny and R.open[cy,cx]
ok=True
def check(name,cond,info=''):
    global ok; ok&=bool(cond); print(('PASS' if cond else 'FAIL'),name,info)
NT=None
import re as _re
hdr=open(os.path.join(os.path.dirname(__file__),'..','private','e1m1_assets.h')).read(); NT=int(_re.search(r'NTHING (\d+)',hdr).group(1))
mons=[i for i in range(NT) if th(i)['kind']<3] if False else None
frames(2)
mons=[i for i in range(NT) if th(i)['kind']<3]; items=[i for i in range(NT) if 3<=th(i)['kind']<=13]
check('estado inicial',rd16('hp')==100 and rd16('ammo')==50 and rd16('px')==1056 and rd16('py')==-3616,f"hp={rd16('hp')} ammo={rd16('ammo')} pos=({rd16('px')},{rd16('py')})")
x0,y0=rd16('px'),rd16('py'); held.add('up'); frames(10); held.clear(); x1,y1=rd16('px'),rd16('py')
check('andar para frente (norte)',y1>y0+20 and abs(x1-x0)<5,f'({x0},{y0})->({x1},{y1})')
y_=rd8('yaw'); held.add('right'); frames(5); held.clear(); check('girar',(rd8('yaw')-y_)&255==15,f"yaw {y_}->{rd8('yaw')}")
# colisao: anda 400 quadros em varias direcoes; precisa permanecer em celulas abertas
bad=0
for yaw in (0,40,90,128,200):
    wr8('yaw',yaw); held.add('up'); 
    for _ in range(40):
        frames(5)
        if not free(rd16('px'),rd16('py')): bad+=1
    held.clear()
check('colisao: jogador nunca entra em celula bloqueada',bad==0,f'violacoes={bad}')
# tiro em monstro: zombie com linha de visao
def place_facing(i,d=140):
    t=th(i)
    for dd in (d,110,90,160,200):
        for a in range(0,360,10):
            cx=t['x']+dd*math.cos(math.radians(a)); cy=t['y']+dd*math.sin(math.radians(a))
            if free(cx,cy) and all(free(t['x']+(cx-t['x'])*k/20,t['y']+(cy-t['y'])*k/20) for k in range(21)):
                wr16('px',int(cx)); wr16('py',int(cy)); 
                wr8('yaw',int(round(math.atan2(t['x']-cx,t['y']-cy)/(2*math.pi)*256))&255); return True
    return False
m0=mons[0]; check('posicao com linha de visao ao monstro',place_facing(m0))
set_th(m0,hp=20); mem[sym['th']+m0*TS+7]=0                        # dormindo
frames(4); n0=rd16('ammo'); k0=rd8('kills')
for k in range(12):
    held.add('fire'); frames(1); held.discard('fire'); frames(14)
    if th(m0)['st']>=4: break
check('tiros abatem o monstro',th(m0)['st'] in (4,5) and rd8('kills')==k0+1,f"st={th(m0)['st']} abates={rd8('kills')} municao {n0}->{rd16('ammo')}")
check('municao diminui por tiro',rd16('ammo')<n0)
# pickup
it=[i for i in items if th(i)['kind']==3 or th(i)['kind']==4][0]; t=th(it)
wr16('hp',50); wr16('px',t['x']); wr16('py',t['y']); frames(3)
check('pegar kit medico/estimulante',rd16('hp')>50 and th(it)['on']==0,f"hp {50}->{rd16('hp')} on={th(it)['on']}")
bonus=[i for i in items if th(i)['kind']==6][0]; t=th(bonus); wr16('px',t['x']); wr16('py',t['y']); a0=rd16('armor'); frames(3)
check('bonus de armadura',rd16('armor')==a0+1,f"armadura {a0}->{rd16('armor')}")
# ataque de monstro: acorda e atira no jogador
m1=mons[1]; place_facing(m1,120); wr16('hp',100); mem[sym['th']+m1*TS+7]=1; set_th(m1,hp=60)
h0=rd16('hp'); frames(200)
check('monstro desperto fere o jogador',rd16('hp')<h0,f'hp {h0}->{rd16("hp")}')
# nukage
sec_dmg=[i for i,s in enumerate(L.m.sectors) if s['special'] in (5,7,16,4)]
ys,xs=[],[]
for s in sec_dmg:
    yy,xx=((R.sec==s)&R.open).nonzero()
    if len(yy): ys,xs=yy,xx; break
cx,cy=R.x0+int(xs[len(xs)//2])*8+4,R.y0+int(ys[len(ys)//2])*8+4
wr16('hp',100); wr16('px',cx); wr16('py',cy)
for it_ in items: set_th(it_,x=0,y=0)
for mm in mons: set_th(mm,x=0,y=0)
frames(80); check('nukage fere',rd16('hp')<100,f'hp 100->{rd16("hp")}')
# saida
wr16('hp',100); ex=int(re.search(r'EXIT_X (-?\d+)',hdr).group(1)); ey=int(re.search(r'EXIT_Y (-?\d+)',hdr).group(1))
wr16('px',ex+50 if free(ex+50,ey) else ex-50); wr16('py',ey); frames(2); held.add('use'); frames(3); held.clear()
check('saida: fase completa ao apertar ENTER perto do interruptor',rd8('over')==2,f"over={rd8('over')}")
# morte e reinicio
wr8('over',0); wr16('hp',0); frames(2); check('fim de jogo ao zerar vida',rd8('over')==1)
held.add('fire'); frames(2); held.clear(); frames(2); check('espaco reinicia',rd8('over')==0 and rd16('hp')==100 and rd16('px')==1056)
print('TODOS OS TESTES PASSARAM' if ok else 'HA FALHAS'); sys.exit(0 if ok else 1)
