"""Puts a 640x480 tank backdrop in the middle of an 854x480 canvas, with the old glass walls and the
new sides transparent, for an image model to fill in. Usage: canvas.py <backdrop> <out.png>"""
import sys
import numpy as np
from PIL import Image

src, out = sys.argv[1:3]
a = np.asarray(Image.open(src).convert('RGBA')).copy()
a[:, :40, 3] = 0
a[:, 600:, 3] = 0
canvas = np.zeros((480, 854, 4), np.uint8)
canvas[:, 107:747] = a
Image.fromarray(canvas).save(out)
