"""Animate the generated product cutout over the static BOLID background."""
from pathlib import Path
from PIL import Image,ImageChops

root=Path(__file__).resolve().parents[1]/'assets/media/bolid'
base=Image.open(root/'bolid-flat-original.png').convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
base.putdata([(0,16,24) if r<10 and g<30 and b<45 else (r,g,b) for r,g,b in base.getdata()])
device=Image.open(root/'s2000-kpb-cutout.png').convert('RGBA')
box=device.getchannel('A').point(lambda a:255 if a>128 else 0).getbbox()
# Match the proportions of the user's original product photograph, 725 x 488.
device=device.crop(box).resize((160,108),Image.Resampling.LANCZOS)
offsets=[0,1,2,3,4,3,2,1,0,-1,-2,-3,-4,-3,-2,-1]
frames=[]
for dy in offsets:
    frame=base.convert('RGBA');frame.alpha_composite(device,(40,183+dy));frames.append(frame.convert('RGB'))
atlas=Image.new('RGB',(240*len(frames),320))
for i,frame in enumerate(frames):atlas.paste(frame,(i*240,0))
palette=atlas.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
indexed=[f.quantize(palette=palette,dither=Image.Dither.NONE) for f in frames]
out=root/'bolid-s2000-kpb-240x320.gif'
indexed[0].save(out,save_all=True,append_images=indexed[1:],duration=120,loop=0,optimize=False,disposal=1)
with Image.open(out) as im:
    assert im.size==(240,320) and im.n_frames==16
    previous=None
    for i in range(im.n_frames):
        im.seek(i);current=im.convert('RGB')
        if previous:
            box=ImageChops.difference(previous,current).getbbox()
            assert box and box[0]>=40 and box[1]>=179 and box[2]<=200 and box[3]<=295,box
        previous=current
assert out.stat().st_size<=262144
indexed[0].save(root/'bolid-s2000-kpb-preview.png')
print(out,out.stat().st_size,'bytes, 16 frames; fixed logo/background, 8px vertical travel')
