"""Boot da ROM no Z80 emulado: confere que a VRAM (linhas 512..1023) e a paleta EPAL ficaram iguais aos dados de origem,
e que o quadro renderizado usa texturas. Classe SIMULATED."""
import os,re,sys
import numpy as np
sys.path.insert(0,os.path.dirname(__file__))
from z80_harness import Machine
R=os.path.join(os.path.dirname(__file__),'..')
M=Machine(lambda row:0xFF); M.run_frames(3)
data=open(os.path.join(R,'out','data_banks.bin'),'rb').read()
src=np.frombuffer(data[:8*16384],np.uint8)
ok=True
def check(n,c,i=''):
    global ok; ok&=bool(c); print('PASS' if c else 'FAIL',n,i)
# 1) regiões da VRAM que o jogo não sobrescreve: atlas de paredes (3 cópias), sprites, gun/fonte
vram=M.vram[512*256:1024*256]
check('VRAM 512..1023 = atlas gravado na ROM',np.array_equal(vram,src),'%d bytes diferentes'%int((vram!=src).sum()))
# 2) paleta EPAL: 256 entradas de 5 bits
h=open(os.path.join(R,'src','assets.h')).read()
pal=np.array([int(x) for x in re.search(r'palette\[768\]=\{([^}]*)\}',h).group(1).split(',')],np.uint8).reshape(256,3)
check('paleta EPAL (256x3 de 5 bits)',np.array_equal(M.pal,(pal.astype(int)*255//31).astype(np.uint8)))
# 3) quadro: texturas aparecem (muitas cores distintas na janela 3D) e HUD presente
f=np.array(M.frames[-1]); win=f[:178].reshape(-1,3); hud=f[178:].reshape(-1,3)
check('janela 3D com >= 40 cores distintas',len(np.unique(win,axis=0))>=40,str(len(np.unique(win,axis=0))))
check('pixels texturizados desenhados',M.log['tex_pixels']>20000,str(M.log['tex_pixels']))
check('HUD desenhado (fundo + texto + barra)',len(np.unique(hud,axis=0))>=6,str(len(np.unique(hud,axis=0))))
ceil=f[:60].reshape(-1,3); check('teto/chao pintados (nao e preto)',(ceil.sum(1)>0).mean()>0.5)
check('nenhuma face em excesso por RUN (<=255)',max(len(M.gf),0)<=255)
# Geo3D ausente (openMSX sem -ext geo3d): a ROM deve avisar na tela em vez de travar em silêncio
N=Machine(lambda row:0xFF,geo=False)
for _ in range(300): N.m.ticks_to_stop=100000; N.m.run()
img=np.array(N.snapshot(0)); view=img[:178].reshape(-1,3)
check('sem Geo3D: mensagem de erro na tela',len(np.unique(view,axis=0))>=3 and N.log['geo_runs']==0,'%d cores, %d RUNs'%(len(np.unique(view,axis=0)),N.log['geo_runs']))
print('bytes enviados ao Geo3D por quadro (média, simulado):',M.log['geo_bytes']//max(1,len(M.frames)-1))
print('RESULTADO:','TODOS OS TESTES PASSARAM' if ok else 'HÁ FALHAS'); sys.exit(0 if ok else 1)
