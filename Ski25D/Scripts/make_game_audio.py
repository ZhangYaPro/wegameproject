"""Original, deterministic winter music and game Foley. No external recordings."""
from pathlib import Path
import json, wave
import numpy as np
from scipy.signal import butter, sosfilt

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ContentSource'/'Audio';OUT.mkdir(parents=True,exist_ok=True)
SR=32000
rng=np.random.default_rng(20271011)
report={}
def noise(seconds,low=180,high=6500):
    return sosfilt(butter(2,[low,high],btype='bandpass',fs=SR,output='sos'),rng.normal(size=int(seconds*SR)))
def tone(midi,seconds,kind='bell'):
    t=np.arange(int(seconds*SR))/SR;f=440*2**((midi-69)/12)
    if kind=='pad':
        y=(np.sin(2*np.pi*f*t)+.22*np.sin(2*np.pi*f*2*t)+.18*np.sin(2*np.pi*f*1.002*t))
        env=np.minimum(t/.25,1)*np.minimum((seconds-t)/.45,1)
    elif kind=='bass':
        y=np.sin(2*np.pi*f*t)+.12*np.sin(2*np.pi*f*2*t);env=(1-np.exp(-t*65))*np.exp(-t*3.8)
    else:
        y=np.sin(2*np.pi*f*t)+.28*np.sin(2*np.pi*f*2*t)*np.exp(-t*3)+.085*np.sin(2*np.pi*f*3*t)*np.exp(-t*5)
        env=(1-np.exp(-t*250))*np.exp(-t*(2.1 if kind=='bell' else 4))
    y*=env;fade=min(len(y)//3,int(.025*SR));y[-fade:]*=np.linspace(1,0,fade)
    return y
def add(buf,sound,start,gain=1,pan=0,wrap=False):
    stereo=sound[:,None]*np.array([np.sqrt((1-pan)/2),np.sqrt((1+pan)/2)])*gain
    offset=int(start*SR)
    if wrap:
        indices=(offset+np.arange(len(sound)))%len(buf);np.add.at(buf,indices,stereo)
    else:
        count=min(len(sound),len(buf)-offset)
        if count>0:buf[offset:offset+count]+=stereo[:count]
def save(name,y,loop=False,target=.14):
    if y.ndim==1:y=np.column_stack([y,y])
    rms=np.sqrt(np.mean(y*y));y*=min(target/max(rms,1e-8),.78/max(np.max(np.abs(y)),1e-8))
    fade=int(SR*(.006 if loop else .012));y[:fade]*=np.linspace(0,1,fade)[:,None];y[-fade:]*=np.linspace(1,0,fade)[:,None]
    pcm=np.round(y*32767).astype('<i2')
    with wave.open(str(OUT/(name+'.wav')),'wb') as w:w.setnchannels(2);w.setsampwidth(2);w.setframerate(SR);w.writeframes(pcm.tobytes())
    report[name]=dict(seconds=round(len(y)/SR,3),peak=round(float(np.max(np.abs(y))),4),rms=round(float(np.sqrt(np.mean(y*y))),4),loop=loop,clipped_samples=int(np.sum(np.abs(y)>=.999)))

# Pentatonic, original four-bar phrases; warm plucks, bell melody and a soft beat.
phrases=[[0,2,4,7,9,7,4,2],[4,7,9,12,9,7,4,7],[9,7,4,2,0,2,4,7],[4,2,0,-3,0,2,0,None]]
for name,bpm,shift,energy in [('MusicRime',96,0,.7),('MusicAlpine',112,2,1),('MusicValley',88,-2,.45)]:
    beat=60/bpm;bars=32;length=bars*4*beat;buf=np.zeros((round(length*SR),2))
    for bar in range(bars):
        chord=[(50,54,57),(47,50,54),(43,47,50),(45,49,52)][bar%4]
        for note in chord:add(buf,tone(note+shift,beat*4+.5,'pad'),bar*4*beat,.042,-.24 if note==chord[0] else .25,True)
        for b in [0,2]:add(buf,tone(chord[0]-12+shift,beat*.9,'bass'),(bar*4+b)*beat,.16,0,True)
        phrase=phrases[(bar//2)%4]
        for k in range(4):
            note=phrase[(bar%2)*4+k]
            if note is not None:
                add(buf,tone(74+shift+note,1.5),(bar*4+k*.875)*beat,.13 if bar%8<6 else .10,.18,True)
            add(buf,tone(chord[k%3]+12+shift,.8,'pluck'),(bar*4+k+.5)*beat,.075,-.25,True)
        if bar>=4:
            for b in [0,2]:
                t=np.arange(int(.22*SR))/SR;kick=np.sin(2*np.pi*(48*t+30*(1-np.exp(-t*28))/28))*np.exp(-t*25)
                add(buf,kick,(bar*4+b)*beat,.075*energy,0,True)
            for b in [1,3]:
                t=np.arange(int(.13*SR))/SR;snare=noise(.13,700,7000)*np.exp(-t*45)
                add(buf,snare,(bar*4+b)*beat,.028*energy,-.12,True)
            for b in np.arange(.5,4,1):
                t=np.arange(int(.08*SR))/SR;add(buf,noise(.08,4000,11000)*np.exp(-t*65),(bar*4+b)*beat,.018*energy,.3,True)
    dry=buf.copy()
    for delay,gain in [(.19,.14),(.31,.10),(.47,.06)]:buf+=np.roll(dry,int(delay*SR),axis=0)[:,::-1]*gain
    save(name,buf,True,.12)

def notes(name,pitches,spacing,duration,gain=.16):
    buf=np.zeros((int(duration*SR),2))
    for i,n in enumerate(pitches):add(buf,tone(n,min(1.1,duration-i*spacing)),i*spacing,.7,(-1)**i*.12)
    save(name,buf,target=gain)
notes('MenuMove',[81],.1,.14,.10)
notes('MenuConfirm',[74,81],.085,.4,.13)
notes('Pickup',[86,93],.055,.38,.12)
notes('Crystal',[81,86,90],.08,.65,.13)
notes('Trick',[74,78,81,86],.085,.85,.14)
notes('Victory',[74,78,81,86,90,86,93],.16,2.3,.16)
notes('Defeat',[62,59,54,50],.23,1.9,.12)
for name,duration,kind in [('Jump',.32,'jump'),('Land',.3,'land'),('Crash',.7,'crash'),('Firework',1.3,'fire')]:
    t=np.arange(int(duration*SR))/SR
    if kind=='jump':y=noise(duration,650,6500)*np.sin(np.pi*t/duration)**1.8*.45
    elif kind=='land':y=noise(duration,100,1900)*np.exp(-t*18)+.32*np.sin(2*np.pi*95*t)*np.exp(-t*24)
    elif kind=='crash':y=noise(duration,130,4400)*np.exp(-t*7)+.3*np.sin(2*np.pi*65*t)*np.exp(-t*14)
    else:y=noise(duration,50,2400)*np.exp(-t*5)+.45*np.sin(2*np.pi*47*t)*np.exp(-t*8)
    save(name,y,target=.15 if kind!='jump' else .10)
for name,low,high in [('SnowGlide',500,6500),('WoodGlide',150,2600)]:
    t=np.arange(SR*8)/SR;y=noise(8,low,high)*(.65+.12*np.sin(2*np.pi*.875*t)+.12*np.sin(2*np.pi*2.125*t))
    save(name,y,True,.11)
(OUT/'audio_manifest.json').write_text(json.dumps(report,indent=2))
assert all(r['clipped_samples']==0 and r['rms']>.02 for r in report.values())
print('ORIGINAL_AUDIO_READY',len(report))
# Preserve the approved full-length recordings when rebuilding Foley.
if (OUT/'Licensed'/'sb_snowfall.mp3').exists() and (OUT/'Licensed'/'sb_freedom.mp3').exists():
    import runpy
    runpy.run_path(str(ROOT/'Scripts'/'prepare_licensed_music.py'))
