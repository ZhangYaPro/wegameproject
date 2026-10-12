"""Prepare credited CC-BY recordings; preserve the existing original game Foley."""
from pathlib import Path
import json, subprocess, wave
import numpy as np

root=Path(__file__).resolve().parents[1];out=root/'ContentSource'/'Audio';sr=32000
manifest=json.loads((out/'audio_manifest.json').read_text())
for name in ['MusicRime','MusicAlpine','MusicValley']:manifest.pop(name,None)
for name,source,excerpt in [('MusicWinter','sb_snowfall.mp3',None),('MusicCelebration','sb_freedom.mp3',(90,154))]:
    raw=subprocess.check_output(['ffmpeg','-v','error','-i',str(out/'Licensed'/source),'-f','f32le','-ac','2','-ar',str(sr),'pipe:1'])
    y=np.frombuffer(raw,dtype='<f4').reshape(-1,2).copy()
    assert len(y)/sr>240, f'Incomplete source recording: {source}'
    if excerpt:
        y=y[int(excerpt[0]*sr):int(excerpt[1]*sr)]
        assert len(y)==64*sr
        # A two-second equal-power join, with no short jingle repeating underneath.
        n=2*sr;t=np.linspace(0,np.pi/2,n)[:,None]
        join=y[-n:]*np.cos(t)+y[:n]*np.sin(t)
        y=np.concatenate([join,y[n:-n]])
    peak=np.max(np.abs(y));rms=np.sqrt(np.mean(y*y))
    y*=min(.16/max(rms,1e-9),.76/max(peak,1e-9))
    if not excerpt:
        n=int(.15*sr);y[:n]*=np.linspace(0,1,n)[:,None];y[-n:]*=np.linspace(1,0,n)[:,None]
    with wave.open(str(out/(name+'.wav')),'wb') as w:
        w.setnchannels(2);w.setsampwidth(2);w.setframerate(sr);w.writeframes(np.round(y*32767).astype('<i2').tobytes())
    manifest[name]=dict(seconds=round(len(y)/sr,3),peak=float(np.max(np.abs(y))),rms=float(np.sqrt(np.mean(y*y))),loop=True,clipped_samples=0,artist='Scott Buckley',license='CC BY 4.0',source=source)
(out/'audio_manifest.json').write_text(json.dumps(manifest,indent=2))
print({k:v['seconds'] for k,v in manifest.items() if k.startswith('Music')})
