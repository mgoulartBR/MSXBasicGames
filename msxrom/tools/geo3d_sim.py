"""Simulador de REFERÊNCIA do modo 'faces preenchidas' do Geo3D (conforme o comentário de
geo3d_engine.v). NÃO é o hardware: valida geometria, winding, culling, ordem do pintor e sombreado."""
import math,sys
from PIL import Image,ImageDraw
sys.path.insert(0,__import__('os').path.dirname(__file__))
import level
W,Hs,F,CX,CY,ZNEAR=256,212,170,128,106,4
PAL=[(0,0,0),(1,0,0),(2,0,0),(3,0,0),(4,0,1),(5,1,1),(6,1,1),(7,2,2),
     (1,1,0),(2,1,0),(3,2,0),(4,3,0),(5,3,1),(6,4,1),(7,5,2),(1,1,2)]
RGB=lambda i:tuple(int(c*255/7) for c in PAL[i])
def view(cam,yaw):
    s,c=math.sin(yaw),math.cos(yaw)
    R=[[c,0,-s],[0,1,0],[s,0,c]]           # linhas: direita, cima, frente
    t=[-sum(R[i][j]*cam[j] for j in range(3)) for i in range(3)]
    return R,t
def run(img,verts,faces,R,t,light=(-0.3,0.64,-0.70),pos=(0,0,0)):
    d=ImageDraw.Draw(img)
    P=[]
    for v in verts:
        w=[v[i]+pos[i] for i in range(3)]
        P.append([sum(R[i][j]*w[j] for j in range(3))+t[i] for i in range(3)])
    LM=[sum(R[i][j]*light[i] for i in range(3)) for j in range(3)]  # transpose(R)*L
    todo=[]
    for idx,n,base in faces:
        pts=[P[i] for i in idx]
        if any(p[2]<ZNEAR for p in pts): continue
        s=[(CX+F*p[0]/p[2],CY-F*p[1]/p[2]) for p in pts]
        area=(s[1][0]-s[0][0])*(s[2][1]-s[0][1])-(s[2][0]-s[0][0])*(s[1][1]-s[0][1])
        if area<=0: continue                       # doc: visível quando area>0
        nn=[x/16384 for x in n]
        lv=min(6,int(7*max(0,sum(LM[i]*nn[i] for i in range(3)))))
        todo.append((sum(p[2] for p in pts),base+lv,s))
    for _,col,s in sorted(todo,key=lambda x:-x[0]): d.polygon(s,fill=RGB(col))
    return len(todo)
def frame(cam,yaw,ents=()):
    img=Image.new('RGB',(W,Hs));d=ImageDraw.Draw(img)
    d.rectangle([0,0,W,CY],fill=RGB(0)); d.rectangle([0,CY,W,Hs],fill=RGB(15))
    R,t=view(cam,yaw); drawn=0;runs=0
    ch=level.build(); ch.sort(key=lambda c:-((c['cx']*64+128-cam[0])**2+(c['cz']*64+128-cam[2])**2))
    for c in ch: drawn+=run(img,c['verts'],c['faces'],R,t); runs+=1
    ev,ef=level.enemy_model()
    for e in sorted(ents,key=lambda e:-((e[0]-cam[0])**2+(e[1]-cam[2])**2)):
        drawn+=run(img,ev,[(i,n,b) for i,n,b in ef],R,t,pos=(e[0],0,e[1])); runs+=1
    return img,drawn,runs
if __name__=='__main__':
    P,E=level.entities()
    img,dr,runs=frame((P[0],32,P[1]),math.radians(40),E)
    img=img.resize((768,636),Image.NEAREST); img.save(sys.argv[1] if len(sys.argv)>1 else 'sim.png')
    print('faces desenhadas',dr,'RUNs',runs)
