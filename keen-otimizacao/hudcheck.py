import sys,glob
from PIL import Image
# score field region of the HUD (640x480 screenshots): plain teal with white digits on the right
bad=[]
fs=sorted(glob.glob(sys.argv[1]+'_*.png'),key=lambda f:float(f.rsplit('_',1)[1][:-4]))
for f in fs:
    im=Image.open(f).convert('RGB')
    cols=set()
    for x in range(420,500):
        for y in range(62,78):cols.add(im.getpixel((x,y)))
    ok={(0,186,186),(0,170,170),(0,148,148)}
    extra=[c for c in cols if c not in ok and not (c[0]>200 and c[1]>200 and c[2]>200) and sum(c)>60]
    if extra:bad.append((f,extra[:3]))
print(len(fs),'shots, flagged:',bad)
