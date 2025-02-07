#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
from typing import List
import re

import uproot
import typer
import numpy
import matplotlib.pyplot


def main(files: List[Path], output: str, title: str = ""):
    mus = []

    for file in files:
        m = re.match(".*mu(\d+).*", file.name)
        mu = int(m.group(1))
        mus.append(mu)

    mus = list(sorted(mus))

    fig, (ax, tax) = matplotlib.pyplot.subplots(
        2, 1, gridspec_kw={"hspace": 0, "height_ratios": [2, 1]}
    )

    for fitter, color in zip(("Iterative", "AMVF"), ["tab:blue", "tab:orange"]):
        mean = numpy.array([])
        stddev = numpy.array([])
        ntrue_mean = numpy.array([])
        ntrue_stddev = numpy.array([])
        time = numpy.array([])

        for mu in mus:
            for file in files:
                if f"mu{mu}" in file.name and fitter in file.name:
                    break

            time_file = file.parent / f"{file.stem}_time.txt"
            if time_file.exists():
                time = numpy.append(time, float(time_file.read_text()))
            else:
                time.append(float("nan"))

            rf = uproot.open(f"{file}:vertexing")

            nreco = rf["nRecoVtx"].array(library="np")
            ntrue = rf["nTrueVtx"].array(library="np")

            nreco_mean = nreco.mean()

            nreco_std = nreco.std()

            mean = numpy.append(mean, nreco_mean)
            stddev = numpy.append(stddev, nreco_std)

            ntrue_mean = numpy.append(ntrue_mean, ntrue.mean())
            ntrue_stddev = numpy.append(ntrue_stddev, ntrue.std())

        ax.fill_between(mus, mean - stddev, mean + stddev, fc=color, alpha=0.2)
        ax.plot(mus, mean, label=fitter, c=color)
        tax.plot(mus, time, label=f"{fitter} time", c=color)

    ax.plot(mus, mus, label="truth", c="gray", ls="--")

    tax.set_xlabel("$\mu$")
    ax.set_ylabel("Number of vertices")
    tax.set_ylabel("wall time [s]")

    fig.align_labels()

    ax.legend()
    tax.legend()
    ax.set_title(title)

    fig.tight_layout()
    fig.savefig(output)


if __name__ == "__main__":
    typer.run(main)
