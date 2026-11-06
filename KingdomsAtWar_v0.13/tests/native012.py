from pathlib import Path
import re,json,hashlib
R=Path(__file__).resolve().parents[1];bounds=list(map(int,re.search(r'pose_bounds\[\]=\{([^}]+)',(R/'src/art07.h').read_text())[1].split(',')));sizes=[32,32,48,32,48,48,32]
def decode(b,n):
 p=0;o=bytearray()
 while len(o)<n:
  flag=b[p];p+=1
  for bit in range(8):
   if len(o)==n:break
   if flag&(1<<bit):o.append(b[p]);p+=1
   else:
    d,l=b[p]+1,b[p+1]+3;p+=2
    for _ in range(l):o.append(o[-d])
 assert len(o)==n
 return o
records=json.loads((R/'art07/runtime/native012.json').read_text());checks=0
for name in ['KAW9968']:
 rom=(R/(name+'.ROM')).read_bytes()
 for rec in records:
  k=rec['key'];raw=(R/f'art07/runtime/pose{k}.bin').read_bytes();n=sizes[k//24]//16;l,t,w,h=bounds[k*4:k*4+4];expected=bytearray()
  for q in range(n*n):
   x=q%n*16;y=q//n*16;left=max(x,l)&~1;right=(min(x+16,l+w)+1)&~1;top=max(y,t);bottom=min(y+16,t+h)
   if left>=right or top>=bottom:continue
   for yy in range(top,bottom):
    a=q*128+(yy-y)*8+(left-x)//2;expected.extend(raw[a:a+(right-left)//2])
  a=rec['bank']*8192+rec['offset'];actual=bytearray(rom[a:a+rec['cropped']]);assert actual==expected and hashlib.sha256(actual).hexdigest()==rec['sha256'];checks+=1
vram=(R/'tests/perf091-V9968-vram.bin').read_bytes();keys=(R/'tests/perf091-V9968-cache.bin').read_bytes();pixels=0
for slot,k in enumerate(keys):
 if k==255:continue
 raw=(R/f'art07/runtime/pose{k}.bin').read_bytes();n=sizes[k//24]//16;l,t,w,h=bounds[k*4:k*4+4]
 for y in range(t,t+h):
  for x in range(l,l+w):
   q=y//16*n+x//16;tile=(slot%7)*9+q;vx=(tile&15)*16+(x&15);vy=(704 if slot<7 else 960)+(tile>>4)*16+(y&15)
   a=vram[vy*128+vx//2];a=a&15 if vx&1 else a>>4;b=raw[q*128+(y&15)*8+(x&15)//2];b=b&15 if x&1 else b>>4
   assert a==b,(slot,k,x,y,a,b);pixels+=1
report={'passed':True,'rom_pose_checks':checks,'actual_vram_pixels':pixels,'original_bytes':sum(x['original']for x in records),'cropped_bytes':sum(x['cropped']for x in records)};(R/'tests/native012-report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',report)
