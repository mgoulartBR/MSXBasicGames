"""Executa private/e1m1.rom no Z80 emulado (harness) e salva quadros. uso: e1m1_sim.py saida_prefixo quadros [x y ang_doom]
(posicao opcional: teleporta o jogador apos o boot, usando o mapa de simbolos de out/e1m1.map)"""
import re,sys,os,math,time
sys.path.insert(0,os.path.dirname(__file__))
from z80_harness import Machine
from PIL import Image
R=os.path.join(os.path.dirname(__file__),'..')
def symbols():
    return {m.group(2):int(m.group(1),16) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',open(os.path.join(R,'out','e1m1.map')).read(),re.M)}
def make(keys_fn=lambda r:0xFF):
    return Machine(keys_fn,rom=os.path.join(R,'private','e1m1.rom'))
if __name__=='__main__':
    pre=sys.argv[1]; n=int(sys.argv[2]); M=make(); sym=symbols(); mem=M.m.memory
    t=time.time(); fr=M.run_frames(2); print('boot ok %.0fs'%(time.time()-t),M.log); mem[sym['tex_planes']]=int(os.environ.get('TEX','0'))
    if len(sys.argv)>5:
        x,y,a=int(sys.argv[3]),int(sys.argv[4]),float(sys.argv[5])
        for nm,v in (('px',x),('py',y)): mem[sym[nm]:sym[nm]+2]=(v&0xFFFF).to_bytes(2,'little')
        mem[sym['yaw']]=int(((90-a)%360)/360*256)&255
    fr=M.run_frames(2+n)
    for i,f in enumerate(fr[1:]): f.resize((768,636),Image.NEAREST).save(f'{pre}_{i}.png')
    print('frames',len(fr),M.log)
