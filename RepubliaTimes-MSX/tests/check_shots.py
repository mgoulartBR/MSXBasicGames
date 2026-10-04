#!/usr/bin/env python3
"""Fail if an expected screenshot is missing or (almost) black/blank: catches boot failures."""
import os, sys
from PIL import Image
import numpy as np
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "screenshots")
expected = sys.argv[1:]
bad = 0
for name in expected:
    p = os.path.join(root, name + ".png")
    if not os.path.exists(p):
        print("MISSING", name); bad += 1; continue
    a = np.array(Image.open(p).convert("L"), dtype=float)
    mean = a.mean()
    ink = (a < 64).mean()
    # a blank/black frame has (almost) no contrast; the night screen is legitimately dark
    ok = a.std() > 40
    print(("ok     " if ok else "BAD    ") + name, "mean=%.0f ink=%.2f std=%.0f" % (mean, ink, a.std()))
    bad += 0 if ok else 1
sys.exit(1 if bad else 0)
