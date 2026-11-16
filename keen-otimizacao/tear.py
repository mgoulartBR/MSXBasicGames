import sys
from PIL import Image
im=Image.open(sys.argv[1]).convert('RGB')
pal={(82,82,82):'8',(0,0,0):'9',(82,255,82):'a',(82,255,255):'b',(255,82,82):'c',(255,82,255):'d',(255,255,82):'e',(255,255,255):'f'}
for L in range(182,194):
    print(L,''.join(pal.get(im.getpixel((x,24+L)),'?') for x in range(40,300,6)))
