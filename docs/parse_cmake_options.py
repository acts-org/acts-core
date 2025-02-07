#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

"""
This script accepts a cmake lists file as an argument extracts all
`option` and `set(... CACHE ...)` variables. It then writes a
markdown table to stdout
"""

import argparse
from pathlib import Path
import re
import textwrap
import sys
import difflib

p = argparse.ArgumentParser(description=__doc__)

p.add_argument("cmakefile", help="Input cmake lists file to parse")
p.add_argument(
    "--prefix", default="ACTS_", help="Prefix to identify relevant variables to extract"
)
p.add_argument(
    "--width",
    type=int,
    default=40,
    help="Width of second column generated from cmake doc strings",
)
p.add_argument(
    "--write",
    "-w",
    help="Write table to this file, expects delimiters CMAKE_OPTS_{BEGIN,END}",
    type=Path,
)
p.add_argument(
    "--verify",
    "-v",
    help="Only verify the target file contains the right table, don't write",
    action="store_true",
)


args = p.parse_args()

cmakefile = Path(args.cmakefile)

with cmakefile.open() as fh:
    opts = {}
    rows = []
    for line in fh:
        if m := re.match(
            rf"option\( *({args.prefix}\w*) \"(.*)\" (ON|OFF|\${{.+}})\ *\) ?(?:# (.*))?.*",
            line,
        ):
            name, doc, default, comment = m.groups()
            # manual default override mechanism
            if comment is not None and comment.strip().startswith("default:"):
                default = comment.strip().split("default:")[1].strip()

            type = "bool"
            if m := re.match(r"\${(\w+)}", default):
                lookup = m.group(1)
                if lookup in opts:
                    default = f"{lookup} -> {opts[lookup]}"
        elif m := re.match(
            rf"set\( *({args.prefix}\w*) \"(.*)\" CACHE (\w+) \"(.*)\"( FORCE)? *\)",
            line,
        ):
            name, default, type, doc, _ = m.groups()
            type = type.lower()
            if default == "":
                default = '""'
        else:
            continue

        opts[name] = default
        doc = "<br>".join(textwrap.wrap(doc, width=args.width))
        rows.append((name, f"{doc}<br> type: `{type}`, default: `{default}`"))

output = ""

headers = ("Option", "Description")
column_lengths = [0] * len(rows[0])

for row in rows:
    for i, col in enumerate(row):
        column_lengths[i] = max(column_lengths[i], len(col))

output += "|"
for i, header in enumerate(headers):
    output += " " + header.ljust(column_lengths[i]) + " |"
output += "\n"

output += "|"
for i in range(len(column_lengths)):
    output += "-" + ("-" * column_lengths[i]) + "-|"
output += "\n"


for row in rows:
    output += "|"
    for i, col in enumerate(row):
        output += " " + col.ljust(column_lengths[i]) + " |"
    output += "\n"

output = output.strip()

if args.write and args.write.exists():
    source = args.write.read_text().split("\n")
    try:
        begin = source.index("<!-- CMAKE_OPTS_BEGIN -->")
        end = source.index("<!-- CMAKE_OPTS_END -->")
    except ValueError:
        print("Markers not found in output file")
        sys.exit(1)

    if args.verify:
        actual = "\n".join(source[begin + 1 : end])
        if output != actual:
            print("MISMATCH:\n" + "-" * 9 + "\n")
            print(
                "\n".join(
                    difflib.unified_diff(
                        actual.split("\n"),
                        output.split("\n"),
                        fromfile="actual",
                        tofile="output",
                    )
                )
            )
            sys.exit(1)
    elif args.write:
        out = source[: begin + 1] + output.split("\n") + source[end:]
        args.write.write_text("\n".join(out))
else:
    print(output)
