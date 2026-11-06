#!/usr/bin/env python3
"""Compara screenshots refA13/refB13 (campo de jogo, sem HUD): scroll incremental x repintura total."""
import sys
from PIL import Image,ImageChops
v=sys.argv[1];a=Image.open(f'screenshots/{v}/refA13.png').convert('RGB').crop((0,0,640,340));b=Image.open(f'screenshots/{v}/refB13.png').convert('RGB').crop((0,0,640,340))
bb=ImageChops.difference(a,b).getbbox();print(v,'campo identico' if bb is None else f'DIFERE {bb}');sys.exit(0 if bb is None else 1)
