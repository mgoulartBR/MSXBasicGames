import sys,glob
from PIL import Image
# count frames where teal HUD-colored pixels appear left/right of the HUD panel (640x480 screenshots)
pre=sys.argv[1];n=0;g=0
for f in sorted(glob.glob(pre+'_*.png'),key=lambda f:float(f.rsplit('_',1)[1][:-4])):
    im=Image.open(f).convert('RGB');n+=1
    def teal(x0,x1,y0,y1):
        c=0
        for x in range(x0,x1,2):
            for y in range(y0,y1,2):
                p=im.getpixel((x,y))
                if p[0]<20 and p[1]>130 and p[2]>130:c+=1
        return c
    if teal(350,408,60,115)+teal(578,612,60,115)>20:g+=1
print(pre,'frames',n,'ghost frames',g)
