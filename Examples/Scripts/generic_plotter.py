#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
from typing import Optional, Dict, List
import re
import enum
import sys

import uproot
import typer
import hist
import pydantic
import yaml
import matplotlib.pyplot
import awkward


class Model(pydantic.BaseModel):
    class Config:
        extra = "forbid"


class HistConfig(Model):
    nbins: int = 100
    min: Optional[float] = None
    max: Optional[float] = None
    label: Optional[str] = None


class Extra(HistConfig):
    expression: str
    name: str


class Config(Model):
    histograms: Dict[str, HistConfig] = pydantic.Field(default_factory=dict)
    extra_histograms: List[Extra] = pydantic.Field(default_factory=list)
    exclude: List[str] = pydantic.Field(default_factory=list)


class Mode(str, enum.Enum):
    recreate = "recreate"
    update = "update"


def main(
    infile: Path = typer.Argument(
        ..., exists=True, dir_okay=False, help="The input ROOT file"
    ),
    treename: str = typer.Argument(..., help="The tree to look up branches from"),
    outpath: Path = typer.Argument(
        "outfile", dir_okay=False, help="The output ROOT file"
    ),
    config_file: Optional[Path] = typer.Option(
        None,
        "--config",
        "-c",
        exists=True,
        dir_okay=False,
        help="A config file following the input spec. By default, all branches will be plotted.",
    ),
    mode: Mode = typer.Option(Mode.recreate, help="Mode to open ROOT file in"),
    plots: Optional[Path] = typer.Option(
        None,
        "--plots",
        "-p",
        file_okay=False,
        help="If set, output plots individually to this directory",
    ),
    plot_format: str = typer.Option(
        "pdf", "--plot-format", "-f", help="Format to write plots in if --plots is set"
    ),
    silent: bool = typer.Option(
        False, "--silent", "-s", help="Do not print any output"
    ),
    dump_yml: bool = typer.Option(False, help="Print axis ranges as yml"),
):
    """
    Script to plot all branches in a TTree from a ROOT file, with optional configurable binning and ranges.
    Also allows setting extra expressions to be plotted as well.
    """

    rf = uproot.open(infile)
    tree = rf[treename]

    outfile = getattr(uproot, mode.value)(outpath)

    if config_file is None:
        config = Config()
    else:
        with config_file.open() as fh:
            config = Config.model_validate(yaml.safe_load(fh))

    histograms = {}

    if not silent:
        print(config.extra_histograms, file=sys.stderr)

    for df in tree.iterate(library="ak", how=dict):
        for col in df.keys():
            if any([re.match(ex, col) for ex in config.exclude]):
                continue
            h = histograms.get(col)
            values = awkward.flatten(df[col], axis=None)

            if len(values) == 0:
                print(f"WARNING: Branch '{col}' is empty. Skipped.")
                continue

            if h is None:
                # try to find config
                found = None
                for ex, data in config.histograms.items():
                    if re.match(ex, col):
                        found = data.model_copy()
                        print(
                            "Found HistConfig",
                            ex,
                            "for",
                            col,
                            ":",
                            found,
                            file=sys.stderr,
                        )

                if found is None:
                    found = HistConfig()

                if found.min is None:
                    found.min = awkward.min(values)

                if found.max is None:
                    found.max = awkward.max(values)

                if found.min == found.max:
                    found.min -= 1
                    found.max += 1

                h = hist.Hist(
                    hist.axis.Regular(
                        found.nbins, found.min, found.max, name=found.label or col
                    )
                )

                histograms[col] = h
            h.fill(values)

            for extra in config.extra_histograms:
                h = histograms.get(extra.name)
                #  calc = pandas.eval(extra.expression, target=df)
                calc = eval(extra.expression)
                values = awkward.flatten(calc, axis=None)
                if h is None:
                    if extra.min is None:
                        extra.min = awkward.min(values)
                    if extra.max is None:
                        extra.max = awkward.max(values)

                if extra.min == extra.max:
                    extra.min -= 1
                    extra.max += 1

                h = hist.Hist(
                    hist.axis.Regular(
                        extra.nbins,
                        extra.min,
                        extra.max,
                        name=extra.label or extra.name,
                    )
                )

                histograms[extra.name] = h
                h.fill(values)

    if plots is not None:
        plots.mkdir(parents=True, exist_ok=True)

    for k, h in histograms.items():
        if not silent:
            if dump_yml:
                ax = h.axes[0]
                s = """
{k}:
  nbins: {b}
  min: {min}
  max: {max}
                """.format(
                    k=k, b=len(ax.edges) - 1, min=ax.edges[0], max=ax.edges[-1]
                )
                print(s)
            else:
                print(k, h.axes[0])
        outfile[k] = h

        if plots is not None:
            fig, ax = matplotlib.pyplot.subplots()

            h.plot(ax=ax, flow=None)

            fig.tight_layout()
            fig.savefig(str(plots / f"{k}.{plot_format}"))
            matplotlib.pyplot.close()


if __name__ == "__main__":
    typer.run(main)
