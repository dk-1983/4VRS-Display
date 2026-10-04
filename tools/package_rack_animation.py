"""Animate small status LEDs over the user's generated 4VRS rack portrait."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageChops

root=Path(__file__).resolve().parents[1]/'assets/media/server-rack'
base=Image.open(root/'rack-original.png').convert('RGB').resize((240,320),Image.Resampling.LANCZOS)
base.putdata([(0,16,24) if r<8 and g<30 and b<45 else (r,g,b) for r,g,b in base.getdata()])
# Front-panel port LEDs, four server rows, and UPS indicator: decorative only.
lights=[(92,97,0),(107,97,1),(127,104,2),(146,104,3),
        (147,157,4),(149,163,5),(147,170,6),(149,177,7),
        (132,215,8),(135,217,9)]
frames=[]
for i in range(24):
    frame=base.copy();draw=ImageDraw.Draw(frame)
    for x,y,k in lights:
        on=((i*(k%3+1)+k*7)%17)<(5 if k<8 else 12)
        if on:
            color=(170,255,75) if k%4 else (255,208,70)
            draw.point((x-1,y),fill=(28,60,18));draw.point((x+1,y),fill=(28,60,18))
            draw.point((x,y-1),fill=(28,60,18));draw.point((x,y+1),fill=(28,60,18))
            draw.point((x,y),fill=color)
        else:draw.point((x,y),fill=(15,45,16))
    frames.append(frame)
atlas=Image.new('RGB',(240*24,320))
for i,f in enumerate(frames):atlas.paste(f,(i*240,0))
palette=atlas.quantize(colors=128,method=Image.Quantize.MEDIANCUT)
frames=[f.quantize(palette=palette,dither=Image.Dither.NONE) for f in frames]
out=root/'4vrs-server-rack-240x320.gif'
frames[0].save(out,save_all=True,append_images=frames[1:],duration=180,loop=0,disposal=1,optimize=False)
with Image.open(out) as im:
    assert im.size==(240,320) and im.n_frames>1
    previous=None;max_rows=0
    for i in range(im.n_frames):
        im.seek(i);current=im.convert('RGB')
        if previous:
            diff=ImageChops.difference(current,previous)
            for y in range(320):
                for x in range(240):
                    if diff.getpixel((x,y))!=(0,0,0):assert any(abs(x-lx)<=1 and abs(y-ly)<=1 for lx,ly,k in lights)
            max_rows=max(max_rows,sum(diff.crop((0,y,240,y+1)).getbbox() is not None for y in range(320)))
        previous=current
assert out.stat().st_size<=262144
frames[0].save(root/'rack-preview.png')
print(out.stat().st_size,'bytes; at most',max_rows,'changed rows; 240x320')
