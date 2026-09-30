"""Losslessly crop native light-theme captures; requires Pillow.

Copyright 2026 Arhythmetic Units. SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
from pathlib import Path
from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture_dir', type=Path)
    parser.add_argument('manual_dir', type=Path)
    args = parser.parse_args()
    # Capture each widget in an 800 x 420 canvas at (10, 20), without scaling.
    images = []
    for module, width in [('RackNES', 570), ('CVGenie', 180)]:
        with Image.open(args.capture_dir / f'{module}-Light.ppm') as source:
            ratio, remainder = divmod(source.width, 800)
            if (source.format != 'PPM' or source.mode != 'RGB' or ratio < 1
                    or remainder or source.height != 420 * ratio):
                raise ValueError('Unexpected capture geometry; review capture.cpp')
            bounds = tuple(n * ratio for n in (10, 20, 10 + width, 400))
            images.append((module, source.crop(bounds)))
    for module, image in images:
        destination = args.manual_dir / module / 'img' / 'Panel.png'
        image.save(destination)
        print(f'Saved {destination}: {image.width} x {image.height}')


if __name__ == '__main__':
    main()
