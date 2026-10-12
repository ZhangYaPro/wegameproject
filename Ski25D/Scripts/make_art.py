"""Generate original, tileable material maps and readable sign artwork (Pillow only)."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import random

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ContentSource' / 'SkiSample'
OUT.mkdir(parents=True, exist_ok=True)
font_path = 'C:/Windows/Fonts/msyh.ttc'
latin_path = 'C:/Windows/Fonts/arial.ttf'

def font(size, latin=False):
    return ImageFont.truetype(latin_path if latin else font_path, size)

def sign(name, eyebrow, title, subtitle, footer):
    im = Image.new('RGB', (1600, 800), '#102f3b')
    d = ImageDraw.Draw(im)
    # Original geometric mountain motif and restrained brass border.
    d.polygon([(870, 660), (1170, 160), (1580, 660)], fill='#1c414d')
    d.polygon([(1070, 325), (1170, 160), (1300, 330), (1200, 300), (1150, 360)], fill='#54727a')
    d.rounded_rectangle((22,22,1578,778), radius=20, outline='#b78e50', width=3)
    d.rectangle((85,93,145,100), fill='#e9b56d')
    d.text((172,69), eyebrow, font=font(31,True), fill='#e9b56d')
    d.text((85,200), title, font=font(108), fill='#f2f5ef')
    d.text((90,360), subtitle, font=font(38), fill='#afc9ca')
    d.line((90,520,1500,520), fill='#526567', width=2)
    d.text((90,574), footer, font=font(35), fill='#e8b56f')
    d.text((92,691), 'JILIN  /  WINTER JOURNEY', font=font(23,True), fill='#809a9f')
    im.save(OUT / f'{name}.png')

sign('Sign_Start', 'CHANGCHUN 2027', '青春相约长春', '第33届世界大学生冬季运动会', '2027.01.15 — 01.25  ·  长春市 / 吉林市')
sign('Sign_Rime', 'JILIN CITY / SONGHUA RIVER', '寒江雪柳', '吉林雾凇 · 把冬日风景装进旅程', '江岸水汽与寒冬相遇，枝头凝结银白风景。')
sign('Sign_Lodge', 'JILIN / WINTER POSTCARD', '白山松水', '风物驿站 · 人间烟火暖雪乡', '向长春2027出发，为青春喝彩。')
sign('Sign_Finish', 'JILIN / A JOURNEY TO REMEMBER', '这一程，吉雪', '青春相约长春 · 冰雪畅游吉林', '雾凇江岸样板  /  按 R 重新出发')
sign('Sign_Changbai', 'CHANGBAI / SNOWY FOREST', '长白听雪', '长白山麓 · 林海与雪峰相伴', '踏雪穿林，收藏白山松水的一抹冬色。')
sign('Sign_Chagan', 'CHAGAN LAKE / WINTER FISHING', '查干冬捕', '松原查干湖 · 冰湖上的丰收记忆', '渔网、冰面与热闹的冬捕，定格吉林年味。')
sign('Sign_Yanbian', 'YANBIAN / A WARM WINTER', '延边暖冬', '朝鲜族民俗 · 雪檐下的人间烟火', '打糕香气与长鼓韵律，为冬日旅途添暖意。')
sign('Sign_Tram', 'CHANGCHUN / CITY MEMORIES', '长春慢时光', '老电车与街灯 · 雪夜中的城市记忆', '从林海滑向灯火，在吉林遇见不同的冬天。')
sign('Sign_Festival', 'CHANGCHUN 2027 / WINTER YOUTH', '青春吉刻', '相约长春2027 · 为冰雪运动喝彩', '让夜空的烟花，点亮这一程吉林雪旅。')

rng = random.Random(2027)
N=256
noise=Image.new('L',(N,N));noise.putdata([rng.randrange(95,230) for _ in range(N*N)])
noise=noise.filter(ImageFilter.GaussianBlur(.7))
noise.save(OUT/'SnowGrain.png')
# Tangent-space normal with a subtle crystalline microstructure.
pix=noise.load();normal=Image.new('RGB',(N,N));p=normal.load()
for y in range(N):
    for x in range(N):
        dx=(pix[(x+1)%N,y]-pix[(x-1)%N,y])*.30
        dy=(pix[x,(y+1)%N]-pix[x,(y-1)%N])*.30
        p[x,y]=(int(128-dx),int(128-dy),250)
normal.save(OUT/'SnowNormal.png')
print(f'Created 6 original textures in {OUT}')

# Original filtered wind noise, looped with a smooth periodic amplitude envelope.
import math, wave, struct
rate=22050
audio_rng=random.Random(711)
low=0.0
samples=[]
for i in range(rate*8):
    low=.965*low+.035*audio_rng.uniform(-1,1)
    envelope=.65+.2*math.sin(2*math.pi*i/(rate*8))+.12*math.sin(2*math.pi*i/(rate*2))
    samples.append(int(max(-1,min(1,low*4*envelope))*16000))
# Crossfade the loop seam.
for i in range(800):
    alpha=i/800
    samples[-800+i]=int(samples[-800+i]*(1-alpha)+samples[i]*alpha)
with wave.open(str(OUT/'WinterWind.wav'),'wb') as w:
    w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate)
    w.writeframes(struct.pack('<'+'h'*len(samples),*samples))
