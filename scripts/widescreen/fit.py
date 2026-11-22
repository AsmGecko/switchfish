"""Scales an image model's 16:9 result to 854x480 for the game, reports how well its middle lines up with the
original, and makes the middle transparent: the game puts the original back there, so the pack only keeps what
the model drew (plus a strip at each seam the game blends and colour matches against).
Usage: fit.py <draft> <backdrop> <out.png> [--full]  (--full: the whole original is the middle, see canvas.py)"""
import sys
import numpy as np
from PIL import Image

KEEP = 24  # px of the model's middle kept inside each seam; the game uses 12

draft, orig, out = sys.argv[1:4]
d = np.asarray(Image.open(draft).convert('RGBA').resize((854, 480), Image.LANCZOS)).copy()
o = np.asarray(Image.open(orig).convert('RGB')).astype(float)
x0, x1, ox0, ox1 = (107, 747, 0, 640) if '--full' in sys.argv else (147, 707, 40, 600)
diff = np.abs(d[:, x0:x1, :3].astype(float) - o[:, ox0:ox1]).mean()
print(f"{out}: middle differs by {diff:.1f} on average (under ~15 blends without a visible seam)")
d[:, :, 3] = 255
d[:, x0 + KEEP:x1 - KEEP] = 0
Image.fromarray(d).save(out, optimize=True)
