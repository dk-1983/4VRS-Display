"""Build a low-bandwidth robot animation from the generated static portrait."""
from pathlib import Path
from PIL import Image,ImageChops
import math

root=Path(__file__).resolve().parents[1]/'assets/media/companion'
base=Image.open(root/'companion-original.png').convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
# Normalize near-background shades to one panel-representable dark color.
base.putdata([(0,16,24) if r<8 and g<30 and b<45 else (r,g,b) for r,g,b in base.getdata()])
eyes=[(66,125,104,146),(136,125,173,146)]
frames=[]
for i in range(24):
    frame=base.copy();pixels=frame.load();source=base.load()
    eye_level=.78+.22*math.cos(i*2*math.pi/24)
    lamp_level=.65+.35*math.cos(i*2*math.pi/24)
    for x0,y0,x1,y1 in eyes+[(110,242,130,258)]:
        level=eye_level if y0<200 else lamp_level
        for y in range(y0,y1):
            for x in range(x0,x1):
                r,g,b=source[x,y]
                if g>75 and b>90 and r<g*.9:
                    pixels[x,y]=(round(r*level),round(g*level),round(b*level))
    frames.append(frame)
atlas=Image.new('RGB',(240*24,320))
for i,f in enumerate(frames):atlas.paste(f,(240*i,0))
palette=atlas.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
indexed=[f.quantize(palette=palette,dither=Image.Dither.NONE) for f in frames]
out=root/'companion-240x320.gif'
indexed[0].save(out,save_all=True,append_images=indexed[1:],duration=120,loop=0,disposal=1,optimize=False)
rows=[]
with Image.open(out) as im:
    assert im.size==(240,320) and im.n_frames>1
    previous=None
    for i in range(im.n_frames):
        im.seek(i);current=im.convert('RGB')
        if previous:
            diff=ImageChops.difference(current,previous)
            changed=[y for y in range(320) if diff.crop((0,y,240,y+1)).getbbox()]
            assert all(125<=y<146 or 242<=y<258 for y in changed)
            rows.append(len(changed))
        previous=current
assert out.stat().st_size<=262144
indexed[0].save(root/'companion-preview.png')
print(out.stat().st_size,'bytes;',min(rows),'..',max(rows),'changed rows')
