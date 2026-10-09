"""Teletransporta o jogador por pontos do E1M1 no Z80 emulado e salva uma folha de contato (private/tour.png).
uso: e1m1_tour.py [x y ang_doom ...]  (sem argumentos: pontos automaticos perto dos monstros e itens)"""
import sys,os,math
sys.path.insert(0,os.path.dirname(__file__))
from e1m1_sim import make,symbols,R
from PIL import Image,ImageDraw
M=make(); sym=symbols(); mem=M.m.memory
def set16(n,v): mem[sym[n]:sym[n]+2]=(v&0xFFFF).to_bytes(2,'little')
def shot(x,y,a):
    set16('px',x); set16('py',y); mem[sym['yaw']]=int(((90-a)%360)/360*256)&255
    n=len(M.frames); M.run_frames(n+4); im=M.frames[-1].copy()
    ImageDraw.Draw(im).text((3,3),'%d,%d,%d'%(x,y,a),fill=(255,255,0)); return im
M.run_frames(2)
pts=[float(v) for v in sys.argv[1:]]
if not pts: pts=[1056,-3616,90, 1500,-3600,0, 2200,-3300,90, 2000,-3000,180, 2900,-3200,0, 2900,-3800,90]
ims=[shot(int(pts[i]),int(pts[i+1]),pts[i+2]) for i in range(0,len(pts),3)]
cols=3; rows=(len(ims)+cols-1)//cols; S=Image.new('RGB',(256*cols,212*rows))
for i,im in enumerate(ims): S.paste(im,((i%cols)*256,(i//cols)*212))
S=S.resize((S.width*2,S.height*2),Image.NEAREST); S.save(os.path.join(R,'private','tour.png')); print(S.size,M.log)
