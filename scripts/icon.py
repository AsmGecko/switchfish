"""Makes the NRO icon (256x256 JPEG) from the guppy on the player's own title screen. Run by sf.sh at build
time; the result stays in build/ and is never committed. Usage: icon.py <titlescreen.jpg> <out.jpg>"""
import sys
from PIL import Image

src, out = sys.argv[1:3]
title = Image.open(src).convert('RGB')
# the guppy in the middle of the 640x480 title screen
title.crop((262, 250, 392, 380)).resize((256, 256), Image.LANCZOS).save(out, quality=92)
