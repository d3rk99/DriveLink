"""Prepare the approved Workshop artwork for the DayZ launcher logo.

Writes Art/logo_ca.png (1024 x 1024 RGBA). The asset build converts it to
Addons/DriveLink/gui/anim/logo_ca.paa, referenced by Mod/mod.cpp.
The full-resolution Workshop preview stays in Art/workshop-cover-v2.png.
"""
import os
import sys
from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE = os.path.join(REPO, 'Art', 'workshop-cover-v2.png')


def logo():
    with Image.open(SOURCE) as source:
        return source.convert('RGBA').resize((1024, 1024), Image.Resampling.LANCZOS)


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, 'Art', 'logo_ca.png')
    logo().save(out)
    print(out)
