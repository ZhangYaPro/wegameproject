from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
out=root/'ContentSource'/'SkiSample'
font=lambda n:ImageFont.truetype('C:/Windows/Fonts/msyh.ttc',n)
# Keep the bold cabin panels and event welcome art as the source of truth.
import runpy
runpy.run_path(str(root/'Scripts'/'make_welcome_art.py'))
for i in range(1,4):
    clean=root/'Saved'/'SkiQA'/f'Art_Chapter_{i}_View_1.png'
    source=clean if clean.exists() else root/'Saved'/'SkiQA'/f'Chapter_{i}_View_1.png'
    im=Image.open(source)
    if not clean.exists():im=im.crop((280,280,1000,670))
    # Use the verified bare-log landing for chapter one; exclude every HUD panel.
    ride=root/'Saved'/'SkiQA'/'Rideable_1.png'
    if i==1 and ride.exists():im=Image.open(ride).crop((110,157,960,617))
    im.resize((1200,650),Image.Resampling.LANCZOS).save(out/f'MenuScene{i}.png')
print('MENU_REVISION_ART_READY')
