"""Check source levels and the actual UE master-bus recording; fix its WAV header."""
from pathlib import Path
import json,wave,sys
import numpy as np
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'ContentSource'/'Audio'/'audio_manifest.json').read_text())
assert len(manifest)==15 and all(x['clipped_samples']==0 for x in manifest.values())
assert manifest['MusicWinter']['seconds']>240 and manifest['MusicCelebration']['seconds']>60
name=sys.argv[1] if len(sys.argv)>1 else 'ExperienceMix'
assert name in ('ExperienceMix','FinaleMix')
p=root/'Saved'/'SkiQA'/(name+'.wav')
with wave.open(str(p),'rb') as w:params=w.getparams();raw=w.readframes(w.getnframes())
# UE's WAV export can write a mono block-align value on a stereo header.
# Re-serialize the identical PCM bytes using a standard, channel-aware header.
with wave.open(str(p),'wb') as w:w.setparams(params);w.writeframes(raw)
y=np.frombuffer(raw,dtype='<i2').astype(np.float64)/32768
report=dict(seconds=params.nframes/params.framerate,channels=params.nchannels,sample_rate=params.framerate,peak=float(np.max(np.abs(y))),rms=float(np.sqrt(np.mean(y*y))),clipped_samples=int(np.sum(np.abs(y)>=.999)),source_assets=len(manifest))
report['passed']=report['seconds']>20 and report['channels']==2 and report['rms']>.005 and report['peak']<.9 and report['clipped_samples']==0
(root/'Saved'/'SkiQA'/('finale_audio_checks.json' if name=='FinaleMix' else 'audio_mix_checks.json')).write_text(json.dumps(report,indent=2))
assert report['passed'],report
print(json.dumps(report))
