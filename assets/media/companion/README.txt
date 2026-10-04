Friendly robot companion animation for 4VRS Display.

companion-240x320.gif: 240x320, infinite loop, 128-color shared palette,
158878 bytes. Only cyan eyes and chest indicator change brightness. Static
body and background reduce display transfer; no full-screen frame movement.
Actual playback speed depends on the display and user speed setting.
Rebuild with Pillow: python tools/package_companion_animation.py

Generated with built-in image_gen. Prompt: friendly white/silver AI robot
portrait, front view, smiling cyan crescent eyes in a dark visor, small cyan
chest indicator, no text, solid dark navy background, static body suitable
for limited-area animation on a 240x320 screen. Original generated image is
preserved in companion-original.png; companion-preview.png is the first frame.
Packaging normalizes near-background shades to a flat RGB565 color and adds
gentle periodic illumination in the eye and chest regions.
