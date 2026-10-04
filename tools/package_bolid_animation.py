"""Slice the generated BOLID frame sheet into a device-sized looping GIF."""
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'assets/media/bolid'
sheet = Image.open(DEST / 'bolid-frames-original.png').convert('RGB')
# Generated sheet: four columns, 339.5-pixel rows, followed by unused padding.
# The first twelve frames describe a complete orbit; the last row repeats it.
frames = []
for i in range(12):
    col, row = i % 4, i // 4
    box = (round(col * sheet.width / 4), round(row * 339.5),
           round((col + 1) * sheet.width / 4), round((row + 1) * 339.5))
    frames.append(sheet.crop(box).resize((240, 320), Image.Resampling.LANCZOS))
palette_source = Image.new('RGB', (240 * len(frames), 320))
for i, frame in enumerate(frames):
    palette_source.paste(frame, (i * 240, 0))
palette = palette_source.quantize(colors=64, method=Image.Quantize.MEDIANCUT)
indexed = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
out = DEST / 'bolid-240x320.gif'
indexed[0].save(out, save_all=True, append_images=indexed[1:], duration=250,
                loop=0, optimize=False, disposal=1)
frames[0].save(DEST / 'bolid-preview.png')
with Image.open(out) as check:
    assert check.size == (240, 320) and check.n_frames == 12
    for i in range(check.n_frames):
        check.seek(i)
        check.load()
assert out.stat().st_size <= 256 * 1024
print(f'{out}: {out.stat().st_size} bytes; 12 frames; 3-second loop')
