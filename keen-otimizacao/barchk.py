import sys,glob
from PIL import Image
ref=Image.open('qr1_10.png').convert('RGB')
R=[[ref.getpixel((32+x,24+184+L)) for x in range(256)] for L in range(8)]
for pre in sys.argv[1:]:
    out=[]
    for f in sorted(glob.glob(pre+'_*.png')):
        im=Image.open(f).convert('RGB')
        out.append(sum(im.getpixel((32+x,24+184+L))!=R[L][x] for L in range(1,8) for x in range(256)))
    print(pre,out)
