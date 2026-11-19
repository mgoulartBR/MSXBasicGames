"""Choose the sprite frames converted to one-pass TIMP images at level load.

At load the VDP rewrites each selected frame's MASK rectangle in place:
    mask' = (~mask & BLACK) ; TIMP-copy of the colour rectangle over it
so mask' is 0 (transparent), the colour index, or BLACK (visible black, index 9).
Frames are converted only when this cannot disturb any other atlas reader:
  * masks are strictly 0/15,
  * the mask rectangle overlaps no other frame's mask/colour rectangle, no tile and no
    other group (frames sharing the identical mask+colour pair are converted once, together).
Everything else stays on the original AND/OR path, so correctness never depends on coverage.
Run after build_opaque_sprites.py and build_tight_sprites.py (needs the final defs and opaque table).
"""
from pathlib import Path
import json,re
root=Path('../vb').resolve()
LEVELS=('scene','melon','bridge','rapids','pass','city','apple','grove','bay','swamp','boobus','castle')
STREAM=('streamed_cart','streamed_object','streamed_aspar','streamed_mouth','streamed_tater','streamed_brocco','streamed_city','streamed_apple','streamed_boobus')
LOAD_BASE={4:7000,5:6500,6:6700,7:6100,8:5000,9:5000,10:5000,11:5000}   # level index -> offset of defs in load_data.bin
CART_FRAME,TOMATO_FRAME=84,90
opaque=[list(map(int,r.split(','))) for r in re.findall(r'\{([0-9,]+)\}',(root/'src/opaque_sprites.h').read_text())]
code=(root/'src/combat_assets.c').read_text()
def special(L,f):
    """Mirror of the early-return dispatch in ActorDraw (src/actors.c)."""
    if L==12 and 116<=f<124:return True
    if L==11 and 108<=f<117:return True
    if L==7 and (108<=f<120 or 136<=f<154):return True
    if L==6 and 16<=f<26:return True
    if L in(6,12) and 152<=f<164:return True
    if (L==2 or (L>=4 and L!=11)) and f in(128,129,134,135):return True
    if ((L==2 or (L>=4 and L!=11)) and 108<=f<116) or ((L==2 or L==6) and 136<=f<140):return True
    if (f<16 and (L!=3 or f not in(0,8))) or f in(30,31) or (36<=f<46 and (L!=3 or (L>=2 and f not in(36,43)))):return True
    if (CART_FRAME<=f<TOMATO_FRAME and not (L>=4 and f==89) and L!=6) or (L>=4 and f==105):return True
    if L in(5,8,9,10) and 136<=f<152:return True
    return False
def defs_for(li,n):
    if li<4:
        t=code[code.index('static const SpriteDef sd%d['%li):];t=t[:t.index(';')]
        raw=[tuple(map(int,v.split(','))) for v in re.findall(r'\{(\d+(?:,\d+){7})\}',t)]
    else:
        b=(root/'assets'/LEVELS[li]/'load_data.bin').read_bytes();o=LOAD_BASE[li]
        raw=[tuple(b[o+i*8:o+i*8+8]) for i in range(n)]
    return [dict(mx=mx,cx=cx,my=my+(256 if ox&128 else 0),cy=cy+(256 if oy&128 else 0),w=w,h=h) for mx,cx,my,cy,w,h,ox,oy in raw]
def inter(a,b):return not(a[0]+a[2]<=b[0] or b[0]+b[2]<=a[0] or a[1]+a[3]<=b[1] or b[1]+b[3]<=a[1])
lists=[];bits=[];report=[];reasons={};GRP={}
for li,lv in enumerate(LEVELS):
    L=li+1;p=root/'assets'/lv
    atlas=(p/'atlas.bin').read_bytes();layout=json.loads((p/'combat_assets.json').read_text())
    sp=layout['sprites'];defs=defs_for(li,len(sp))
    def px(x,y):
        v=atlas[y*128+x//2];return v&15 if x&1 else v>>4
    tiles=[(x,y,16,16) for x,y in layout['tiles']]
    frames={}
    for f,(s,d) in enumerate(zip(sp,defs)):
        if (s['mask']==[0,0] and s['color']==[0,0]) or any(s.get(k) for k in STREAM) or d['w']==0 or d['h']==0:continue
        isop=bool(opaque[li][f]);M=(d['mx'],d['my'],d['w'],d['h']);C=(d['cx'],d['cy'],d['w'],d['h'])
        frames[f]=dict(M=M,C=C,opaque=isop,special=special(L,f))
    groups={}
    for f,v in frames.items():groups.setdefault((v['M'],v['C']),[]).append(f)
    chosen=[];chosen_frames=[]
    groupof={}
    for k,fs in groups.items():
        for f in fs:groupof[f]=k
    GRP[li]=(frames,groups)
    for (M,C),fs in groups.items():
        if any(frames[f]['opaque'] or frames[f]['special'] for f in fs):
            for f in fs:reasons[(li,f)]='opaque/special' if frames[f]['special'] or frames[f]['opaque'] else 'group-has-opaque/special'
            continue
        ok=all({px(M[0]+x,M[1]+y) for y in range(M[3]) for x in range(M[2])}<={0,15} for _ in [0])
        why=None if ok else 'mask-not-0/15'
        for (M2,C2),fs2 in groups.items():
            if (M2,C2)==(M,C):continue
            if inter(M,M2) and not all(frames[f]['opaque'] for f in fs2) or inter(M,C2):
                ok=False;why='mask-shared/overlaps other frame (M2 %s C2 %s)'%('Y' if inter(M,M2) else 'n','Y' if inter(M,C2) else 'n');break
        if ok and any(inter(M,t) for t in tiles):ok=False;why='overlaps tile'
        if ok and inter(M,C):ok=False;why='mask overlaps own colour'
        if ok:chosen.append(fs[0]);chosen_frames+=fs
        else:
            for f in fs:reasons[(li,f)]=why
    b=bytearray(21)
    for f in chosen_frames:b[f>>3]|=1<<(f&7)
    lists.append(sorted(chosen));bits.append(bytes(b))
    cand=sum(1 for v in frames.values() if not v['opaque'] and not v['special'])
    report.append((lv,'masked frames',cand,'converted',len(chosen_frames)))

import sys
for L,files in ((4,'fs_4'),(8,'fs_8'),(12,'fs_12'),(10,'fs_10'),(7,'fs_7'),(6,'fs_6')):
    li=L-1
    try:rows=[l.split() for l in open(files+'.txt.fr')]
    except:print('no',L);continue
    use={int(a):int(b) for a,b in rows}
    tot=sum(use.values());conv=set(lists[li])
    chosen_frames=set()
    frames,groups=GRP[li]
    for k,fs in groups.items():
        if fs[0] in conv:chosen_frames|=set(fs)
    print('== level',L,'draws',tot)
    byreason={}
    for f,n in sorted(use.items(),key=lambda x:-x[1]):
        if f in chosen_frames:r='CONVERTED(TIMP)'
        elif f not in frames:r='not masked frame / streamed / special-dispatch'
        else:r=reasons.get((li,f),'?')
        byreason[r]=byreason.get(r,0)+n
    for r,n in sorted(byreason.items(),key=lambda x:-x[1]):print(f'   {100*n/tot:4.0f}%  {r}  ({n})')
    print('   top unconverted frames:',[(f,n,reasons.get((li,f),'-')) for f,n in sorted(use.items(),key=lambda x:-x[1]) if f not in chosen_frames][:8])

print('\n##### Strategy B (convert the COLOUR rect in place via scratch) feasibility, draw-weighted #####')
def inter2(a,b):return not(a[0]+a[2]<=b[0] or b[0]+b[2]<=a[0] or a[1]+a[3]<=b[1] or b[1]+b[3]<=a[1])
for L,files in ((4,'fs_4'),(8,'fs_8'),(12,'fs_12'),(10,'fs_10'),(7,'fs_7'),(6,'fs_6')):
    li=L-1
    use={int(a):int(b) for a,b in (l.split() for l in open(files+'.txt.fr'))}
    tot=sum(use.values());frames,groups=GRP[li]
    p=root/'assets'/LEVELS[li];atlas=(p/'atlas.bin').read_bytes();layout=json.loads((p/'combat_assets.json').read_text())
    tiles=[(x,y,16,16) for x,y in layout['tiles']]
    def px2(x,y):
        v=atlas[y*128+x//2];return v&15 if x&1 else v>>4
    conv=set()
    for k,fs in groups.items():
        if fs[0] in lists[li]:conv|=set(fs)
    featB=set();why={}
    for (M,C),fs in groups.items():
        if any(frames[f]['opaque'] or frames[f]['special'] for f in fs) or fs[0] in lists[li]:continue
        if not {px2(M[0]+x,M[1]+y) for y in range(M[3]) for x in range(M[2])}<={0,15}:why[fs[0]]='mask impure';continue
        bad=None
        for (M2,C2),fs2 in groups.items():
            if (M2,C2)==(M,C):continue
            if inter2(C,C2) or inter2(C,M2):
                if not all(frames[f]['opaque'] for f in fs2) or inter2(C,C2):bad='C overlaps other frame';break
        if not bad and any(inter2(C,t) for t in tiles):bad='C overlaps tile'
        if not bad and inter2(C,M):bad='C overlaps own M'
        if bad:why[fs[0]]=bad
        else:featB|=set(fs)
    a=sum(n for f,n in use.items() if f in conv);b=sum(n for f,n in use.items() if f in featB)
    print(f'L{L}: draws {tot}  converted now {100*a/tot:.0f}%  +strategy B {100*b/tot:.0f}%  (remaining masked-unconverted: {100*(sum(n for f,n in use.items() if f in frames and f not in conv and f not in featB and not frames[f]["opaque"] and not frames[f]["special"]))/tot:.0f}%)')

print('\n##### remaining blockers under B (top drawn frames) #####')
for L,files in ((8,'fs_8'),(4,'fs_4'),(12,'fs_12')):
    li=L-1;use={int(a):int(b) for a,b in (l.split() for l in open(files+'.txt.fr'))}
    frames,groups=GRP[li];grp_of={}
    for k,fs in groups.items():
        for f in fs:grp_of[f]=k
    conv=set()
    for k,fs in groups.items():
        if fs[0] in lists[li]:conv|=set(fs)
    print('L',L)
    for f,n in sorted(use.items(),key=lambda x:-x[1])[:10]:
        if f in conv or f not in frames:continue
        (M,C)=grp_of[f]
        sharedM=[g for g in groups if g!=(M,C) and inter2(M,g[0])]
        sharedC=[g for g in groups if g!=(M,C) and (inter2(C,g[1]) or inter2(C,g[0]))]
        print(f'  frame {f} draws {n} size {M[2]}x{M[3]} M={M[:2]} C={C[:2]} groupsize={len(groups[(M,C)])} Mhits={len(sharedM)} Chits={len(sharedC)}', 'Cwho',[(g[0][:2],g[1][:2]) for g in sharedC][:2])
