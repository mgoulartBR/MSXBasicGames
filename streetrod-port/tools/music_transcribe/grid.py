import librosa, numpy as np
y,sr=librosa.load('in.wav',sr=22050)
hop=128
oenv=librosa.onset.onset_strength(y=y,sr=sr,hop_length=hop,aggregate=np.median)
t=librosa.frames_to_time(np.arange(len(oenv)),sr=sr,hop_length=hop)
best=None
for step in np.arange(0.1436,0.1456,0.00002):
    for off in np.arange(0,step,0.002):
        idx=np.round((np.arange(0,int((t[-1]-off)/step))*step+off)/ (t[1]-t[0])).astype(int)
        idx=idx[idx<len(oenv)]
        s=oenv[idx].sum()/len(idx)
        if best is None or s>best[0]: best=(s,step,off)
print(best)
