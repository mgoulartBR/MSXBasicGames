import sys,glob
from PIL import Image
for pre in sys.argv[1:]:
    r=[]
    for f in sorted(glob.glob(pre+'_*.png')):
        im=Image.open(f).convert('RGB')
        # width of black run from x=32 on line 184 beyond normal 8px margin (game lines have black x<32+? )
        def run(L):
            n=0
            while im.getpixel((32+n,24+L))==(0,0,0):n+=1
            return n
        r.append((run(184),run(186)))
    print(pre,r)
