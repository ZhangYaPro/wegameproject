"""Original chapter wayfinding, matching the existing authored sign system."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
out=Path(__file__).resolve().parents[1]/'ContentSource'/'SkiSample'
font=lambda n:ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',n)
rows=[
('Chapter1_Entry','01  /  RIME RIVERSIDE','雾凇江岸','寒江雪柳 · 从长春开启冰雪旅程','长缓坡与雪村相伴，轻按起跳、长按空翻。',0),
('Chapter1_End','NEXT  /  BEIDAHU','下一站，北大湖','雾凇江岸完成 · 前往开阔雪场','到达终点后按空格进入第二关。',0),
('Chapter2_Entry','02  /  BEIDAHU CHALLENGE','北大湖挑战','吉林市 · 缆车、飞坡与青春雪场','看清起跳旗与落地区，向更漂亮的动作出发。',1),
('Chapter2_Lift','BEIDAHU  /  SKI RESORT','向雪山出发','缆车掠过山坡 · 吉林冰雪运动','松开空格就近回正，稳稳落地再加分。',1),
('Chapter2_Youth','CHANGCHUN 2027','青春驰骋冰雪','为大学生冰雪运动加油','原创应援场景 · 本赛道为艺术化设计。',1),
('Chapter2_End','NEXT  /  VOLCANIC CANYON','下一站，长白岩谷','北大湖挑战完成 · 穿行蓝冰峡谷','到达终点后按空格进入第三关。',1),
('Chapter3_Entry','03  /  CHANGBAI VALLEY','长白岩谷','覆雪山脊 · 冰瀑与火山雪境','以长白山火山地貌为灵感的虚构雪道。',2),
('Chapter3_Ravine','CHANGBAI  /  BLUE ICE','穿过蓝冰峡谷','火山岩壁与冰雪 · 吉林自然之美','提前观察落地区，合理起跳与收势。',2),
('Chapter3_Lodge','JILIN  /  WARM WINTER','雪深处，有灯火','峡谷驿站 · 白山松水的人间温度','玄岩、蓝冰与暖灯，收藏吉林的冬日记忆。',2),
('Chapter3_End','JILIN  /  THREE CHAPTERS','把吉林，收进冬天','雾凇江岸 → 北大湖 → 长白岩谷','青春相约长春2027 · 三关冰雪之旅。',2)]
for name,brow,title,subtitle,footer,chapter in rows:
    bg,accent=(('#143d48','#e9bf7b'),('#103a62','#ff9858'),('#173b37','#cbb27e'))[chapter]
    im=Image.new('RGB',(1600,800),bg);d=ImageDraw.Draw(im)
    d.rounded_rectangle((20,20,1580,780),radius=20,outline=accent,width=4)
    if chapter==1:
        d.line((1120,170,1550,90),fill='#739cbe',width=5)
        for x,y in [(1240,145),(1460,105)]:
            d.line((x,y,x,y+65),fill=accent,width=5);d.rounded_rectangle((x-62,y+65,x+62,y+158),radius=15,fill='#467da4')
    else:
        for x,y in [(1180,160),(1390,115),(1510,230)]:
            d.polygon([(x,y),(x-140,y+270),(x+140,y+270)],fill='#3c6460' if chapter==2 else '#4c707b')
    d.text((85,65),brow,font=font(32),fill=accent)
    d.text((85,202),title,font=font(105),fill='#f2f5ef')
    d.text((90,365),subtitle,font=font(38),fill='#c1d8da')
    d.line((90,525,1510,525),fill=accent,width=2)
    d.text((90,575),footer,font=font(32),fill=accent)
    d.text((90,697),'JILIN  /  WINTER JOURNEY',font=font(24),fill='#9bbaba')
    im.save(out/(name+'.png'))
print('CHAPTER_ART_READY',len(rows))
