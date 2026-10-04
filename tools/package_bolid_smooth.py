"""Package a fixed generated background with a small procedural animation layer."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageChops
import math
import argparse
parser=argparse.ArgumentParser();parser.add_argument('--flat',action='store_true');args=parser.parse_args()

root=Path(__file__).resolve().parents[1]/'assets/media/bolid'
base=Image.open(root/('bolid-flat-original.png' if args.flat else 'bolid-static-original.png')).convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
if args.flat:
    # Collapse near-background AI output colors before GIF palette allocation.
    # The chosen solid color is exactly representable by the panel's RGB565.
    base.putdata([(0,16,24) if r<10 and g<30 and b<45 else (r,g,b) for r,g,b in base.getdata()])
scale=3
layer=Image.new('RGBA',(240*scale,320*scale))
d=ImageDraw.Draw(layer)
d.ellipse((77*scale,180*scale,163*scale,266*scale),outline=(0,100,140,255),width=scale)
base=Image.alpha_composite(base.convert('RGBA'),layer.resize(base.size,Image.Resampling.LANCZOS)).convert('RGB')
frames=[]
for i in range(32):
    layer=Image.new('RGBA',(720,960));d=ImageDraw.Draw(layer)
    angle=2*math.pi*i/32-math.pi/2
    for k in range(14):
        a=angle-k*.018
        x=(120+43*math.cos(a))*scale;y=(223+43*math.sin(a))*scale
        strength=1-k/14
        for radius,alpha in [(4,18),(2.5,40),(1.2,220)]:
            r=radius*scale
            d.ellipse((x-r,y-r,x+r,y+r),fill=(int(130*strength),220,255,int(alpha*strength)))
    frames.append(Image.alpha_composite(base.convert('RGBA'),layer.resize(base.size,Image.Resampling.LANCZOS)).convert('RGB'))
atlas=Image.new('RGB',(240*32,320))
for i,f in enumerate(frames):atlas.paste(f,(240*i,0))
pal=atlas.quantize(colors=96,method=Image.Quantize.MEDIANCUT)
frames=[f.quantize(palette=pal,dither=Image.Dither.NONE) for f in frames]
out=root/('bolid-flat-v2-240x320.gif' if args.flat else 'bolid-smooth-240x320.gif')
frames[0].save(out,save_all=True,append_images=frames[1:],duration=80,loop=0,disposal=1,optimize=False)
with Image.open(out) as im:
    assert im.size==(240,320) and im.n_frames==32
    decoded=[]
    for i in range(im.n_frames):im.seek(i);decoded.append(im.convert('RGB'))
    rows=[]
    for i in range(32):
        diff=ImageChops.difference(decoded[i],decoded[(i-1)%32]);box=diff.getbbox()
        assert box and box[0]>=70 and box[1]>=174 and box[2]<=170 and box[3]<=273,box
        rows.append(sum(diff.crop((0,y,240,y+1)).getbbox() is not None for y in range(320)))
assert out.stat().st_size<262144
frames[0].save(root/('bolid-flat-preview.png' if args.flat else 'bolid-smooth-preview.png'))
print(f'{out.stat().st_size} bytes, 32 frames; changed rows {min(rows)}..{max(rows)} instead of 320')
