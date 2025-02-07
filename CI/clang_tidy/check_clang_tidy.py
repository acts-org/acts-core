#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import argparse
import sys
from pathlib import Path
import json
import itertools
import fnmatch
import re

import yaml
from rich.console import Console, Group
from rich.text import Text
from rich.panel import Panel
from rich.rule import Rule
from rich.emoji import Emoji
from rich.table import Table


from item import ItemCollection


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--report", type=Path, required=True)
    p.add_argument("--config", type=Path, required=True)
    p.add_argument("--strip-prefix-path", type=Path)

    args = p.parse_args()

    console = Console()

    with args.config.open() as fh:
        config = yaml.safe_load(fh)

    data = []
    with args.report.open() as fh:
        data = ItemCollection(root=json.load(fh)).root
        for item in data:
            if args.strip_prefix_path and not item.path.is_absolute:
                item.path = item.path.relative_to(args.strip_prefix_path)

    counts = config["limits"].copy()

    kf = lambda i: i.path
    for file, items in itertools.groupby(sorted(data, key=kf), key=kf):
        output = []
        for item in items:
            emoji = Emoji(
                {"warning": "yellow_circle", "error": "red_circle"}[item.severity]
            )

            style = "bold "
            if item.severity == "warning":
                style += "yellow"
            elif item.severity == "error":
                style += "red"

            s = Text()
            s.append(f"{emoji}")
            s.append(f" {item.path}:{item.line}:{item.col}", style="bold")
            s.append(f" {item.severity.upper()} ", style=style)
            s.append("[")
            s.append(item.code, style="bold")
            s.append(f"]")

            def subpath(m):
                return f"[bold]{m.group(1)}[/bold]:"

            message = re.sub(r"([\w/.\-+]+:\d+:\d+):", subpath, item.message)

            accounted_for = False
            for pattern in config["limits"].keys():
                if not fnmatch.fnmatch(item.code, pattern):
                    continue
                counts[pattern] += 1
                accounted_for = True

            if accounted_for:
                output.append(s)
                output.append(Panel(message))
                output.append(Rule())
                pass
            else:
                counts.setdefault(item.code, 0)
                counts[item.code] += 1

        #  output = output[:-1]
        if len(output) > 0:
            console.print(Panel(Group(*output), title=str(file)))

    table = Table()
    table.add_column("", width=2)
    table.add_column("code / pattern")
    table.add_column("count", justify="right")
    table.add_column("limit", justify="right")
    exit = 0
    for pattern, count in counts.items():
        limit = config["limits"].get(pattern, float("inf"))
        emoji = Emoji("green_circle")
        style = "green"
        if limit == float("inf"):
            emoji = Emoji("white_circle")
            style = "white"
        elif count > limit:
            exit = 1
            emoji = Emoji("red_circle")
            style = "red bold"
        table.add_row(emoji, pattern, str(count), str(limit), style=style)

    console.rule()
    console.print(Panel.fit(table, title="Results"), justify="center")

    if exit != 0:
        console.print(
            Panel(
                Text(f"{Emoji('red_circle')} FAILURE", justify="center"),
                style="red bold",
            )
        )
    else:
        console.print(
            Panel(
                Text(f"{Emoji('green_circle')} SUCCESS", justify="center"),
                style="green bold",
            )
        )

    sys.exit(exit)


if "__main__" == __name__:
    main()
