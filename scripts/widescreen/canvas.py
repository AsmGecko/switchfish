"""Puts a 640x480 tank backdrop's middle (inside its glass walls) on an 854x480 canvas, the rest transparent,
for an image model to fill in, glass walls included. Usage: canvas.py <backdrop> <out.png>"""
import sys
import numpy as np
from PIL import Image

src, out = sys.argv[1:3]
a = np.asarray(Image.open(src).convert('RGBA'))
canvas = np.zeros((480, 854, 4), np.uint8)
canvas[:, 147:707] = a[:, 40:600]
Image.fromarray(canvas).save(out)
