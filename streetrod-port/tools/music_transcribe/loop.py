import librosa, numpy as np, json
C=np.load('cqt36.npy'); times=np.load('times36.npy'); bpo=36
freqs=librosa.cqt_frequencies(bpo*5,fmin=librosa.note_to_hz('C2'),bins_per_octave=bpo)
midi=librosa.hz_to_midi(freqs)
STEP=0.14496; OFF=0.092; S0=7; N=191; LOOP=27.6956
def spec_at(i):
    out=[]
    for rep in (0,1):
        a=OFF+(S0+i)*STEP+rep*LOOP; b=a+STEP
        seg=C[:,(times>=a+0.03)&(times<b-0.005)]
        if seg.shape[1]: out.append(np.median(seg,axis=1))
    return np.mean(out,axis=0)
def voices(spec,nv=4,thr=0.8):
    s=spec.copy(); out=[]
    for _ in range(nv):
        k=int(np.argmax(s))
        if s[k]<thr: break
        out.append([round(float(midi[k])),float(s[k])])
        for h in (1,2,3,4,5,6):
            kk=k+int(round(bpo*np.log2(h)))
            if kk<len(s): s[max(0,kk-4):kk+5]*=0.0
    # drop semitone-neighbour leakage of a stronger voice
    res=[]
    for m,e in out:
        if any(abs(m-m2)==1 and e2>2.5*e for m2,e2 in res): continue
        if any(abs(m-m2)==1 for m2,e2 in res): continue
        res.append((m,e))
    return res
rows=[voices(spec_at(i)) for i in range(N)]
json.dump(rows,open('loop.json','w'))
nm=lambda m: librosa.midi_to_note(m).replace('♯','#')
for bar in range(0,12):
    print('--- bar',bar+1)
    line=[]
    for i in range(bar*16,min(N,bar*16+16)):
        line.append(f'{i%16:2d}:'+','.join(f'{nm(m)}' for m,s in rows[i]))
    print('  '.join(line))
