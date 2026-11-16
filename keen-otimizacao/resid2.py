import sys,glob
from PIL import Image
ref=Image.open('qr1_10.png').convert('RGB')
R=[[ref.getpixel((x,24+L)) for x in range(32,288)] for L in range(185,192)]
for pre in sys.argv[1:]:
    fs=sorted(glob.glob(pre+'_*.png'));out=[]
    for f in fs:
        im=Image.open(f).convert('RGB')
        out.append(sum(im.getpixel((32+x,24+185+L))!=R[L][x] for L in range(7) for x in range(256)))
    print(pre,out)
