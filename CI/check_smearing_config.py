#!/usr/bin/env python3

# This file is part of the ACTS project.
#
# Copyright (C) 2016 CERN for the benefit of the ACTS project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Configuration taken from: https://arxiv.org/pdf/1904.06778.pdf
# See also https://github.com/acts-project/acts/issues/946

import argparse
import math
import subprocess

args = argparse.ArgumentParser()
args.add_argument("sourcedir")
args = args.parse_args()

volumes = [7, 8, 9, 12, 13, 14, 16, 17, 18]


base_cli = [
    "python",
    args.sourcedir + "/Examples/Algorithms/Digitization/scripts/smearing-config.py",
]

for vid in volumes:
    # 50μm×50μmand further out two different strip detectors withshort80μm×1200μmand long strips0.12 mm×10.8 mmare placed.
    if vid in [7, 8, 9]:  # Pixel
        resx, resy = 0.05, 0.05
    elif vid in [12, 13, 14]:  # Short strip
        resx, resy = 0.08, 1.2
    elif vid in [16, 17, 18]:  # Long strip
        resx, resy = 0.12, 10.8
    else:
        raise RuntimeError("Invalid volume id")

    resx /= math.sqrt(12)
    resy /= math.sqrt(12)

    base_cli += [
        "--digi-smear-volume={}".format(vid),
        "--digi-smear-indices=0:1",
        "--digi-smear-types=0:0",
        "--digi-smear-parameters={}:{}".format(resx, resy),
    ]

output = subprocess.check_output(base_cli).decode("utf-8")

ref_path = (
    args.sourcedir
    + "/Examples/Algorithms/Digitization/share/default-smearing-config-generic.json"
)

with open(ref_path, "r") as ifile:
    ref = ifile.read()

for i, (line_ref, line_gen) in enumerate(zip(ref.split("\n"), output.split("\n"))):
    lhs = line_ref.strip()
    rhs = line_gen.strip()

    if lhs != rhs:
        raise RuntimeError(f"Mismatched line #{i}: Ref=<{lhs}>, Gen=<{rhs}>")
