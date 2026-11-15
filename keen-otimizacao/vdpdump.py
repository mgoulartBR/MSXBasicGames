import sys
t0=float(sys.argv[1]);t1=float(sys.argv[2]);page=int(sys.argv[3]);f=sys.argv[4]
names={0:'STOP',4:'POINT',5:'PSET',6:'SRCH',7:'LINE',8:'LMMV',9:'LMMM',10:'LMCM',11:'LMMC',12:'HMMV',13:'HMMM',14:'YMMM',15:'HMMC'}
for l in open(f):
    p=l.split();t=float(p[0]);r=list(map(int,p[1:]))
    if not(t0<=t<t1):continue
    sx=r[0]+256*(r[1]&1);sy=r[2]+256*(r[3]&3);dx=r[4]+256*(r[5]&1);dy=r[6]+256*(r[7]&3);nx=r[8]+256*(r[9]&3);ny=r[10]+256*(r[11]&3)
    cmd=r[14]>>4;op=r[14]&15;arg=r[13]
    ddy=dy-page*256
    if len(sys.argv)>5 and not (0<=ddy<=70 and dx<=200 and dx+nx>=60):continue
    print('%.4f %s%s src(%d,%d) dst(%d,%d) n(%dx%d) arg=%d clr=%d'%(t,names.get(cmd,cmd),'' if cmd not in(9,8) else '/op%d'%op,sx,sy,dx,dy,nx,ny,arg,r[12]))
