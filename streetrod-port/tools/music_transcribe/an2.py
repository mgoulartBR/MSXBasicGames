import librosa, numpy as np, sys
y,sr=librosa.load('in.wav',sr=22050)
hop=128
bpo=36
C=np.abs(librosa.cqt(y,sr=sr,hop_length=hop,fmin=librosa.note_to_hz('C2'),n_bins=bpo*5,bins_per_octave=bpo,filter_scale=1.2))
times=librosa.frames_to_time(np.arange(C.shape[1]),sr=sr,hop_length=hop)
np.save('cqt36.npy',C); np.save('times36.npy',times)
freqs=librosa.cqt_frequencies(bpo*5,fmin=librosa.note_to_hz('C2'),bins_per_octave=bpo)
midi=librosa.hz_to_midi(freqs)
def voices(spec,n=3):
    s=spec.copy(); out=[]
    for _ in range(n):
        k=int(np.argmax(s));
        if s[k]<0.15: break
        out.append((midi[k],s[k]))
        for h in (1,2,3,4,5):
            kk=k+int(round(bpo*np.log2(h)))
            if kk<len(s): s[max(0,kk-3):kk+4]*=0.0
    return out
step=float(sys.argv[2]) if len(sys.argv)>2 else 0.14532
off=float(sys.argv[1])
for i in range(0,64):
    a=off+i*step; b=a+step
    seg=C[:,(times>=a+0.03)&(times<b)]
    if seg.shape[1]==0: continue
    v=voices(np.median(seg,axis=1))
    print(f'{i:3d} {a:6.2f}',' '.join(f'{librosa.midi_to_note(round(m))}({m:.1f}):{s:.1f}' for m,s in v))
