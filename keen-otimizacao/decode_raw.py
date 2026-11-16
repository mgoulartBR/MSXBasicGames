import sys
from PIL import Image
im=Image.open(sys.argv[1]).convert('RGB')
pal={(82,82,82):'r70',(82,82,255):'r71',(82,255,82):'r75',(82,255,255):'r77',(255,82,82):'r78',(255,82,255):'r79',(255,255,82):'r86',(255,255,255):'r87',(0,0,0):'zero'}
print([pal.get(im.getpixel((int(sys.argv[2]) if len(sys.argv)>2 else 200,24+L)),'?%s'%(im.getpixel((200,24+L)),)) for L in range(14)])
