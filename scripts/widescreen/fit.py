"""Scales an image model's 16:9 result to 854x480 for the game and reports how well its middle lines up
with the original (the game puts the original middle back pixel exact). Usage: fit.py <draft> <backdrop> <out.png>"""
import sys
import numpy as np
from PIL import Image

draft, orig, out = sys.argv[1:4]
d = np.asarray(Image.open(draft).convert('RGB').resize((854, 480), Image.LANCZOS))
o = np.asarray(Image.open(orig).convert('RGB')).astype(float)
diff = np.abs(d[:, 147:707].astype(float) - o[:, 40:600]).mean()
print(f"{out}: middle differs by {diff:.1f} on average (under ~15 blends without a visible seam)")
Image.fromarray(d).save(out)
