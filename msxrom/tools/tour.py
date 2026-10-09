"""Teletransporta o jogador por pontos da fase no Z80 emulado e salva uma folha de contato (out/tour.png)."""
import re,sys,os,math
sys.path.insert(0,os.path.dirname(__file__))
from z80_harness import Machine
from PIL import Image,ImageDraw
R=os.path.join(os.path.dirname(__file__),'..')
sym={m.group(2):int(m.group(1),16) for m in re.finditer(r'^\s+([0-9A-F]{8})\s+_(\w+)\s',open(os.path.join(R,'out','msxdoom.map')).read(),re.M)}
M=Machine(lambda r:0xFF); mem=M.m.memory
def set16(n,v): mem[sym[n]:sym[n]+2]=(v&0xFFFF).to_bytes(2,'little')
def shot(x,z,deg,label):
    set16('px',x); set16('pz',z); mem[sym['yaw']]=int(deg/360*256)&255
    n=len(M.frames); M.run_frames(n+8); im=M.frames[-1].copy()
    ImageDraw.Draw(im).text((3,3),label,fill=(255,255,0)); return im
c=lambda x,z:(x*64+32,z*64+32)
views=[(*c(3,22),0,'inicio (sala SO)'),(*c(4,15),90,'corredor -> salao'),(*c(6,12),90,'salao: oeste do acido'),
       (*c(12,8),180,'passarela, de cima'),(*c(12,11),0,'sobre a passarela'),(*c(18,12),270,'salao: leste'),
       (*c(21,8),0,'corredor da saida'),(*c(20,5),90,'sala de saida')]
ims=[shot(*v) for v in views]
S=Image.new('RGB',(256*3,212*3))
for i,im in enumerate(ims[:9]): S.paste(im,((i%3)*256,(i//3)*212))
S=S.resize((S.width*2//1,S.height*2//1),Image.NEAREST); S.save(os.path.join(R,'out','tour.png')); print(S.size)
