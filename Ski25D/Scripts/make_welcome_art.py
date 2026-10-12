"""Large, short cabin lettering and an original Changchun 2027 welcome board."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1];out=root/'ContentSource'/'SkiSample'
font=lambda n:ImageFont.truetype('C:/Windows/Fonts/msyhbd.ttc',n)
def centred(d,text,cy,size,colour,width):
    f=font(size);b=d.textbbox((0,0),text,font=f)
    while b[2]-b[0]>width-90:size-=2;f=font(size);b=d.textbbox((0,0),text,font=f)
    d.text(((width-b[2]+b[0])/2-b[0],cy-(b[3]-b[1])/2-b[1]),text,font=f,fill=colour)
for name,title in [('GondolaJilin','雾凇岛'),('GondolaBeidahu','长白山'),
                   ('GondolaJingyuetan','净月潭'),('GondolaChagan','查干湖'),
                   ('GondolaColdNoodles','延吉冷面'),('GondolaRiceCake','打糕'),
                   ('GondolaGinseng','人参'),('GondolaKimchi','辣白菜')]:
    im=Image.new('RGB',(1024,580),'#12394a');d=ImageDraw.Draw(im)
    d.rectangle((12,12,1011,567),outline='#f6bd64',width=12)
    for x in [40,358,676]:
        d.rounded_rectangle((x,34,x+308,112),radius=8,fill='#78b9cd')
        d.line((x+10,43,x+135,43),fill='#c7e7ed',width=5)
    centred(d,title,344,390 if len(title)==2 else 305,'#fff8da',1024)
    im.save(out/(name+'.png'))
im=Image.new('RGB',(2048,1024),'#103a50');d=ImageDraw.Draw(im)
d.rounded_rectangle((22,22,2026,1002),radius=24,outline='#f8c76c',width=8)
d.rectangle((50,50,1998,58),fill='#70d5d2')
centred(d,'欢迎相约吉林长春',212,202,'#fff8df',2048)
centred(d,'2027 · 第33届',421,121,'#ffcd78',2048)
centred(d,'世界大学生冬季运动会',602,154,'#fff8df',2048)
centred(d,'2027.01.15 — 01.25',807,82,'#a7e8e3',2048)
centred(d,'共赴冰雪盛会 · 为世界青年喝彩',923,65,'#d7e9e7',2048)
im.save(out/'Welcome2027.png')
im.resize((1600,800),Image.Resampling.LANCZOS).save(out/'Chapter2_Youth.png')
print('WELCOME_AND_CABIN_ART_READY')
