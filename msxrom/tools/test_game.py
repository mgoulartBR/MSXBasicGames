"""Testes de lógica da ROM no Z80 emulado (SIMULATED). Lê variáveis da RAM via out/msxdoom.map."""
import re,sys,os,math
sys.path.insert(0,os.path.dirname(__file__))
from z80_harness import Machine
sym={m.group(2):int(m.group(1),16) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',open(os.path.join(os.path.dirname(__file__),'..','out','msxdoom.map')).read(),re.M)}
KEYS={'left':0x10,'right':0x80,'up':0x20,'down':0x40,'fire':0x01}
held=set()
def kb(row):
    if row!=8: return 0xFF
    v=0xFF
    for k in held: v&=~KEYS[k]
    return v
M=Machine(kb)
def rd8(n): return M.m.memory[sym[n]]
def rd16(n): v=M.m.memory[sym[n]]|M.m.memory[sym[n]+1]<<8; return v-65536 if v>32767 else v
def wr8(n,v): M.m.memory[sym[n]]=v
def frames(n):
    t=len(M.frames); M.run_frames(t+n)
ok=True
def check(name,cond,info=''):
    global ok; ok&=bool(cond); print(('PASS' if cond else 'FAIL'),name,info)
frames(2)
check('estado inicial',rd16('hp')==100 and rd8('alive_count')==9,f"hp={rd16('hp')} vivos={rd8('alive_count')}")
x0,z0=rd16('px'),rd16('pz'); held.add('up'); frames(10); held.clear()
x1,z1=rd16('px'),rd16('pz'); check('andar para frente',abs(x1-x0)+abs(z1-z0)>10,f'({x0},{z0})->({x1},{z1})')
y0=rd8('yaw'); held.add('right'); frames(5); held.clear(); check('girar',(rd8('yaw')-y0)&255==15,f"yaw {y0}->{rd8('yaw')}")
held.add('up'); frames(120); held.clear()   # empurra contra parede
px,pz=rd16('px'),rd16('pz'); check('colisão com parede (dentro do mapa)',0<px<1024 and 0<pz<1024,f'({px},{pz})')
# mira no inimigo mais próximo e atira
ex=[(rd16('en')+0,0)]  # placeholder para usar o símbolo
base=sym['en']; ES=14; mem=M.m.memory
def enemy(i):
    o=base+i*ES; g=lambda k:(lambda v:v-65536 if v>32767 else v)(mem[o+k]|mem[o+k+1]<<8); return g(0),g(2),g(4)
px,pz=rd16('px'),rd16('pz'); wr8('yaw',0)
alive=[i for i in range(9) if enemy(i)[2]>0]
ex_,ez_,_=enemy(alive[0]); 
# escolhe uma posição livre a 60-90 unidades do inimigo, com linha de visão, e mira nele
import level
def free(x,z): return not level.solid(int(x)>>6,int(z)>>6)
def find_pos(ex,ez):
    for d in (60,70,80,90):
        for a in range(0,360,15):
            cx=ex+d*math.cos(math.radians(a)); cz=ez+d*math.sin(math.radians(a))
            if all(free(ex+(cx-ex)*t/12,ez+(cz-ez)*t/12) for t in range(0,13)) and free(cx,cz): return int(cx),int(cz)
cx,cz=find_pos(ex_,ez_)
mem[sym['px']:sym['px']+2]=(cx&0xFFFF).to_bytes(2,'little'); mem[sym['pz']:sym['pz']+2]=(cz&0xFFFF).to_bytes(2,'little')
wr8('yaw',int(round(math.atan2(ex_-cx,ez_-cz)/(2*math.pi)*256))&255)
frames(6); M.frames[-1].resize((768,636)).save(os.path.join(os.path.dirname(__file__),'..','out','preview_enemy.png'))
hp0=rd16('hp'); n0=rd8('alive_count'); hits=0
for k in range(3):
    held.add('fire'); frames(1); held.discard('fire'); frames(12)
    hits=3-0
frames(2)
e_hp=[enemy(i)[2] for i in range(9)]
check('tiro abate inimigo (3 disparos, 3 de vida)',rd8('alive_count')==n0-1,f'vivos {n0}->{rd8("alive_count")} hp_inimigos={e_hp}')
# derrota: zera vida
mem[sym['hp']:sym['hp']+2]=(0).to_bytes(2,'little'); frames(2)
check('fim de jogo ao zerar vida',rd8('over')==1)
held.add('fire'); frames(1); held.clear(); frames(2)
check('reinício com SPACE',rd8('over')==0 and rd16('hp')>=90 and rd8('alive_count')==9)
print('RESULTADO:','TODOS OS TESTES PASSARAM' if ok else 'HÁ FALHAS'); sys.exit(0 if ok else 1)
