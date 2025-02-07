#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import sys

from headwind.spec import CollectorResult, Metric

import pandas as pd

assert len(sys.argv) == 2
input_file = sys.argv[1]


metrics = []
metric_names = []

df = pd.read_csv(input_file)
df = df.drop_duplicates(subset=["file"])
df.max_rss = df.max_rss / 1024 / 1024 / 1024  # GB

limit = 20

mdf = df.sort_values(by="max_rss", ascending=False)
for row in mdf.itertuples():
    name = "max_rss_" + row.file
    metrics.append(
        Metric(name=name, value=row.max_rss, unit="GB", group="compile_max_rss")
    )
    if name in metric_names:
        print("Duplicate:", name)
    metric_names.append(name)

tdf = df.sort_values(by="time", ascending=False)
for row in tdf.itertuples():
    name = "time_" + row.file
    metrics.append(
        Metric(name=name, value=row.time, unit="seconds", group="compile_time")
    )
    if name in metric_names:
        print("Duplicate:", name)
    metric_names.append(name)

sys.stdout.write(CollectorResult(metrics=metrics).json(indent=2))
