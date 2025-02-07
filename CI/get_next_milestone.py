#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import os
import urllib.request
import json
import sys

GITHUB_TOKEN = os.environ["GH_TOKEN"]
GITHUB_REPO = os.environ["GH_REPO"]

with urllib.request.urlopen(
    urllib.request.Request(
        f"https://api.github.com/repos/{GITHUB_REPO}/milestones?state=open",
        headers={"Authorization": f"Bearer {GITHUB_TOKEN}"},
    )
) as response:
    milestones = json.loads(response.read())


sys.stderr.write(f"Found {len(milestones)} milestones\n")
for m in milestones:
    sys.stderr.write(f"- {m['title']}\n")


titles = [m["title"] for m in milestones if m["title"] != "next"]
titles.sort()

if len(titles) == 0:
    raise RuntimeError("No eligible milestone found")

print(titles[0])
