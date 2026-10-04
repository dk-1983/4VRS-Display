BOLID display animation

bolid-240x320.gif: 240 x 320, 12 frames, 250 ms per frame, infinite loop,
64-color global palette, 254051 bytes. Intended for firmware media library.
Device playback can be slower than the encoded timing at 1 MHz SPI.
bolid-preview.png is the first frame.
bolid-frames-original.png preserves the generated source sheet.
Rebuild with Pillow: python tools/package_bolid_animation.py

Artwork generated with the built-in image_gen tool from the user's BOLID logo.
Design prompt: preserve BOLID wordmark and Russian caption "СИСТЕМЫ БЕЗОПАСНОСТИ";
near-black navy background, metallic cyan logo, cyan framing corner accents,
thin circular halo with traveling cyan arc, gentle illumination, no additional
status text. Request a 4 x 4 animation sheet with fixed composition and a
seamless orbit. The generated sheet contains a repeated final row; packaging
uses the first twelve frames, resized to the device's 240 x 320 canvas.

BOLID branding belongs to its respective owner. This personal display artwork
does not imply affiliation or endorsement; the project MIT license does not
grant rights to third-party trademarks.

Optimized variants (2026-10-05):
bolid-smooth-240x320.gif: fixed background, small procedural orbit layer;
32 frames, 38266 bytes, measured around 5 fps on the 1 MHz display.
bolid-flat-v2-240x320.gif: solid RGB565-representable dark navy background,
32 frames, 26335 bytes. package_bolid_smooth.py --flat rebuilds this variant.
The flat and smooth original PNGs were edited with built-in image_gen:
remove the circle for a static background, then remove background gradients
while preserving the BOLID wordmark, caption and corner accents.

bolid-s2000-kpb-240x320.gif: the user's S2000-KPB product floats vertically
under the fixed BOLID logo, replacing the ring; 16 frames, 92642 bytes.
package_bolid_device.py builds it from bolid-flat-original.png and
s2000-kpb-cutout.png. The latter was generated with built-in image_gen using
the user's photo: remove only the surrounding background, preserve the front
view, case, indicator lights and labels, use transparent background.
The animation layer travels 8 pixels total; source photo proportions retained
at 160 x 108 pixels. Device playback timing is hardware-dependent.
