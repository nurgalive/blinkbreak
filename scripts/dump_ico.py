import sys
from PIL import Image

def dump_ico(path):
    img = Image.open(path)
    print(f"File: {path}")
    frames = getattr(img, 'n_frames', 1)
    print(f"Number of frames: {frames}")
    for i in range(frames):
        img.seek(i)
        print(f"Frame {i}: size={img.size}")

dump_ico('resources/bb_logo_blue.ico')

