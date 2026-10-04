"""Package a static Tux portrait with small eye-blink and badge-glint layers."""
from pathlib import Path
from PIL import Image,ImageDraw
import math
root=Path(__file__).resolve().parents[1]/'assets/media/tux'
base=Image.open(root/'tux-original.png').convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
base.putdata([(0,16,24) if r<8 and g<30 and b<45 else (r,g,b) for r,g,b in base.getdata()])
frames=[]
for i in range(24):
    f=base.copy();d=ImageDraw.Draw(f)
    closed={12:.45,13:1,14:1,15:.45}.get(i,0)
    if closed:
        for x0,x1 in [(95,112),(126,144)]:
            mask=Image.new('L',base.size);m=ImageDraw.Draw(mask);m.ellipse((x0,128,x1,154),fill=255)
            m.rectangle((0,round(128+26*closed),240,320),fill=0)
            f.paste((18,19,22),(0,0,240,320),mask)
            if closed==1:ImageDraw.Draw(f).arc((x0,139,x1,146),0,180,fill=(55,58,63),width=1)
    d=ImageDraw.Draw(f);level=(1+math.cos(i*math.pi/12))/2
    if level>.15:
        c=(int(120*level),int(220*level),int(255*level))
        d.line((176,45,176,49),fill=c);d.line((174,47,178,47),fill=c)
    frames.append(f)
atlas=Image.new('RGB',(240*24,320))
for i,f in enumerate(frames):atlas.paste(f,(240*i,0))
palette=atlas.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
frames=[f.quantize(palette=palette,dither=Image.Dither.NONE) for f in frames]
out=root/'tux-intel-inside-240x320.gif'
frames[0].save(out,save_all=True,append_images=frames[1:],duration=160,loop=0,disposal=1,optimize=False)
with Image.open(out) as im:
    assert im.size==(240,320) and im.n_frames>1
    for i in range(im.n_frames):im.seek(i);im.load()
assert out.stat().st_size<=262144
frames[0].save(root/'tux-preview.png')
print(out.stat().st_size,'bytes, 240x320, looping blink and small glint')
