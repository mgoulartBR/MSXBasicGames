import re,sys
from PIL import Image,ImageDraw,ImageFont
S='/tmp/claude-0/-home-user-MSXBasicGames/cebb32c5-1abc-529f-a253-d0e39cbf9a57/scratchpad/bench/'
OUT='/home/user/MSXBasicGames/keen-otimizacao/release-0.9.103/graficos/'
def load(f):
    d={}
    for l in open(S+f):
        m=re.match(r'L(\d+) (\S+) total=([\d.]+)',l)
        if m:d[(int(m[1]),m[2])]=float(m[3])
    return d
new=load('ckst5.done');old=load('ckst5.main')
v99=[old[(i,'vb99')] for i in range(1,13)];v103=[new[(i,'vb')] for i in range(1,13)]
v75=[9.1,6.8,5.1,6.8,10.1,4.6,6.7,3.7,8.7,7.0,4.6,6.3]
names=['Hill','Mines','Bridge','Rapids','Pass','City','Apple','Grove','Bay','Swamp','Boobus','Castle']
BG=(17,20,27);FG=(232,234,240);MUT=(160,166,180);GRID=(40,45,56);GREY=(122,132,146);TEAL=(60,200,190);RED=(232,110,100)
def font(sz,bold=False):
    return ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf'%('-Bold' if bold else ''),sz)
def grouped(fn,title,sub,note,la,lb,A,B,ymax):
    W,H=1280,720;im=Image.new('RGB',(W,H),BG);d=ImageDraw.Draw(im)
    d.text((60,36),title,font=font(32),fill=FG);d.text((60,86),sub,font=font(24),fill=TEAL);d.text((60,126),note,font=font(15),fill=MUT)
    # legend
    x=W-330
    for lab,c in ((la,GREY),(lb,TEAL)):
        d.rectangle((x,44,x+26,64),fill=c);d.text((x+36,42),lab,font=font(20),fill=FG);x+=150
    L,R,T,Bm=90,W-50,200,668
    step=4 if ymax<=24 else 5
    for v in range(0,int(ymax)+1,step):
        y=Bm-(Bm-T)*v/ymax;d.line((L,y,R,y),fill=GRID,width=1);d.text((L-12,y-9),str(v),font=font(16),fill=MUT,anchor='ra')
    gw=(R-L)/12;bw=gw*0.36
    for i in range(12):
        gx=L+gw*i+gw*0.12
        for k,(val,c) in enumerate(((A[i],GREY),(B[i],TEAL))):
            x0=gx+k*bw;y0=Bm-(Bm-T)*val/ymax
            d.rectangle((x0,y0,x0+bw-2,Bm),fill=c)
            d.text((x0+bw/2-1,y0-4),'%.1f'%val,font=font(13),fill=FG,anchor='md')
        d.text((gx+bw,Bm+8),names[i],font=font(16),fill=FG,anchor='ma')
    im.save(fn)
def delta(fn):
    W,H=1280,720;im=Image.new('RGB',(W,H),BG);d=ImageDraw.Draw(im)
    d.text((60,36),'Keen Dreams MSX2 - change in frames per second',font=font(32),fill=FG)
    d.text((60,86),'0.9.99 -> 0.9.103, Z80 at 5.37 MHz (1.5x)',font=font(24),fill=TEAL)
    d.text((60,126),'openMSX, controlled-position script. Hill/Boobus dips are trajectory noise (standing still: equal). openMSX only.',font=font(15),fill=MUT)
    pct=[(b-a)/a*100 for a,b in zip(v99,v103)]
    L,R,T,Bm=190,W-80,190,690;rowh=(Bm-T)/12
    lo,hi=-15,40;zx=L+(R-L)*(0-lo)/(hi-lo)
    for v in range(lo,hi+1,5):
        x=L+(R-L)*(v-lo)/(hi-lo);d.line((x,T-6,x,Bm),fill=GRID,width=1);d.text((x,T-28),('%+d%%'%v if v else '0'),font=font(14),fill=MUT,anchor='ma')
    for i,p in enumerate(pct):
        y=T+rowh*i+rowh*0.18;h=rowh*0.64;x1=L+(R-L)*(p-lo)/(hi-lo)
        c=TEAL if p>=0 else RED
        d.rectangle((min(zx,x1),y,max(zx,x1),y+h),fill=c)
        d.text((L-14,y+h/2),names[i],font=font(17),fill=FG,anchor='rm')
        tx=(x1+8) if p>0.05 else (zx+8)
        d.text((tx,y+h/2),'%+.1f%%  (%.1f -> %.1f)'%(p,v99[i],v103[i]),font=font(14),fill=FG,anchor='lm')
    d.line((zx,T-6,zx,Bm),fill=MUT,width=2)
    im.save(fn)
if __name__=='__main__':
    grouped(OUT+'fps-0.9.99-vs-0.9.103-5.37MHz.png','Keen Dreams MSX2 - frames per second','Z80 at 5.37 MHz (1.5x) - 0.9.99 vs 0.9.103','openMSX, controlled-position script (renders/s). Not tested on real hardware.','0.9.99','0.9.103',v99,v103,20)
    grouped(OUT+'fps-0.9.75-vs-0.9.103-5.37MHz.png','Keen Dreams MSX2 - frames per second','Z80 at 5.37 MHz (1.5x) - 0.9.75 vs 0.9.103','openMSX, controlled-position script (renders/s). Not tested on real hardware.','0.9.75','0.9.103',v75,v103,20)
    delta(OUT+'fps-delta-0.9.99-vs-0.9.103.png')
    print(['%+.1f'%((b-a)/a*100) for a,b in zip(v99,v103)]);print(v99);print(v103)
