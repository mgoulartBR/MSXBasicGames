from pathlib import Path
import re
R=Path(__file__).resolve().parents[1]
p=R/'src/combat_assets.c';s=p.read_text().replace('{0,0,0,0,48,16,0,0}','{0,0,0,0,72,32,0,0}');p.write_text(s)
for name,base in [('pass',7000),('city',6500),('grove',6100),('bay',5000),('swamp',5000),('boobus',5000),('castle',5000),('apple',6700)]:
 p=R/'assets'/name/'load_data.bin'
 if p.exists():
  b=bytearray(p.read_bytes());off=base+35*8
  assert b[off+4:off+6] in (bytes([48,16]),bytes([72,32])),(name,list(b[off:off+8]))
  b[off+4:off+6]=bytes([72,32]);p.write_bytes(b)
 if name=='apple':
  # The ROM packs Apple's definitions from map_part0..4 (2048-byte slices of load_data.bin);
  # patching only load_data.bin left the HUD at 48x16 in the shipped ROM (ghost HUD pieces).
  for i in range(5):(R/'assets/apple'/('map_part%d.bin'%i)).write_bytes(bytes(b[i*2048:min(len(b),(i+1)*2048)]))
print('HUD bounds: 72 x 32 on all stage definitions')
p=R/'assets/rapids/ram_sprites.bin'
b=bytearray(p.read_bytes());b[35*8+4:35*8+6]=bytes([72,32]);p.write_bytes(b)
