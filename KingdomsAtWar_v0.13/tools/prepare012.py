"""Lossless native sprite packing: omit bytes outside approved drawing bounds."""
from pathlib import Path
import ast,re,json,hashlib
r=Path(__file__).resolve().parents[1];source=(r/'tools/prepare09.py').read_text();tree=ast.parse(source);scope={};exec(ast.get_source_segment(source,next(x for x in tree.body if isinstance(x,ast.FunctionDef) and x.name=='lz')),scope);lz=scope['lz']
p=r/'src/art07.h';text=p.read_text();bounds=list(map(int,re.search(r'pose_bounds\[\]=\{([^}]+)',text)[1].split(',')));sizes=[32,32,48,32,48,48,32];chunks=[bytearray()];refs=['{0,0}']*168;records=[]
for k in range(168):
 pth=r/f'art07/runtime/pose{k}.bin'
 if not pth.exists():continue
 raw=pth.read_bytes();n=sizes[k//24]//16;l,t,w,h=bounds[k*4:k*4+4];packed=bytearray()
 for q in range(n*n):
  x=q%n*16;y=q//n*16;left=max(x,l)&~1;right=(min(x+16,l+w)+1)&~1;top=max(y,t);bottom=min(y+16,t+h)
  if left>=right or top>=bottom:continue
  for yy in range(top,bottom):
   a=q*128+(yy-y)*8+(left-x)//2;packed.extend(raw[a:a+(right-left)//2])
 encoded=lz(packed)
 if len(chunks[-1])+len(encoded)>8192:chunks.append(bytearray())
 bank=15+len(chunks);off=len(chunks[-1]);refs[k]='{%d,0x%04X}'%(bank,0xA000+off);chunks[-1].extend(encoded);records.append(dict(key=k,bank=bank,offset=off,original=len(raw),cropped=len(packed),compressed=len(encoded),sha256=hashlib.sha256(packed).hexdigest()))
assert 16+len(chunks)<=25
for bank,data in enumerate(chunks,16):(r/f'src/banks/art_s{bank}_b3.c').write_text('#include "msxgl.h"\nconst u8 nativebank_%d[]={\n'%bank+'\n'.join(','.join(map(str,data[i:i+32]))+','for i in range(0,len(data),32))+'\n};\n')
(r/'src/native012.h').write_text('const Asset poses[]={'+','.join(refs)+'};\n')
if '#include "native012.h"' not in text:p.write_text(re.sub(r'(const Asset poses\[\]=\{.*?\};)',r'#ifdef KAW_V9990\n\1\n#else\n#include "native012.h"\n#endif',text))
(r/'art07/runtime/native012.json').write_text(json.dumps(records,indent=2)+'\n');print('native bytes',sum(x['original']for x in records),'->',sum(x['cropped']for x in records))
