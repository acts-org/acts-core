#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import argparse

p = argparse.ArgumentParser()
p.add_argument("output")
p.add_argument("files", nargs="+")

args = p.parse_args()

items = {}

for file in args.files:
    with open(file) as fh:
        for line in fh:
            key, value = line.split(":", 1)
            items[key.strip()] = value.strip()

with open(args.output, "w") as fh:
    for key, value in items.items():
        fh.write(f"{key}: {value}\n")
