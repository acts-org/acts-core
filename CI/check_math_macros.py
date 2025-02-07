#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
import os
import argparse
from fnmatch import fnmatch
import re
import sys


math_constants = [
    ("M_PI", "std::numbers::pi"),
    ("M_PI_2", "std::numbers::pi / 2."),
    ("M_PI_4", "std::numbers::pi / 4."),
    ("M_1_PI", "std::numbers::inv_pi"),
    ("M_2_PI", "2. * std::numbers::inv_pi"),
    ("M_2_SQRTPI", "2. * std::numbers::inv_sqrtpi"),
    ("M_E", "std::numbers::e"),
    ("M_LOG2E", "std::numbers::log2e"),
    ("M_LOG10E", "std::numbers::log10e"),
    ("M_LN2", "std::numbers::ln2"),
    ("M_LN10", "std::numbers::ln10"),
    ("M_SQRT2", "std::numbers::sqrt2"),
    ("M_SQRT1_2", "1. / std::numbers::sqrt2"),
    ("M_SQRT3", "std::numbers::sqrt3"),
    ("M_INV_SQRT3", "std::numbers::inv_sqrt3"),
    ("M_EGAMMA", "std::numbers::egamma"),
    ("M_PHI", "std::numbers::phi"),
]


github = "GITHUB_ACTIONS" in os.environ


def handle_file(
    file: Path, fix: bool, math_const: tuple[str, str]
) -> list[tuple[int, str]]:
    ex = re.compile(rf"(?<!\w){math_const[0]}(?!\w)")

    content = file.read_text()
    lines = content.splitlines()

    changed_lines = []

    for i, oline in enumerate(lines):
        line, n_subs = ex.subn(rf"{math_const[1]}", oline)
        lines[i] = line
        if n_subs > 0:
            changed_lines.append((i, oline))

    if fix and len(changed_lines) > 0:
        file.write_text("\n".join(lines) + "\n")

    return changed_lines


def main():
    p = argparse.ArgumentParser()
    p.add_argument("input", nargs="+")
    p.add_argument("--fix", action="store_true", help="Attempt to fix M_* macros.")
    p.add_argument("--exclude", "-e", action="append", default=[])

    args = p.parse_args()

    exit_code = 0

    inputs = []

    if len(args.input) == 1 and os.path.isdir(args.input[0]):
        # walk over all files
        for root, _, files in os.walk(args.input[0]):
            root = Path(root)
            for filename in files:
                # get the full path of the file
                filepath = root / filename
                if filepath.suffix not in (
                    ".hpp",
                    ".cpp",
                    ".ipp",
                    ".h",
                    ".C",
                    ".c",
                    ".cu",
                    ".cuh",
                ):
                    continue

                if any([fnmatch(str(filepath), e) for e in args.exclude]):
                    continue

                inputs.append(filepath)
    else:
        for file in args.input:
            inputs.append(Path(file))

    for filepath in inputs:
        for math_const in math_constants:
            changed_lines = handle_file(
                file=filepath, fix=args.fix, math_const=math_const
            )
            if len(changed_lines) > 0:
                exit_code = 1
                print()
                print(filepath)
                for i, oline in changed_lines:
                    print(f"{i}: {oline}")

                    if github:
                        print(
                            f"::error file={filepath},line={i+1},title=Do not use macro {math_const[0]}::Replace {math_const[0]} with std::{math_const[1]}"
                        )

    if exit_code == 1 and github:
        print(f"::info You will need in each flagged file #include <numbers>")

    return exit_code


if "__main__" == __name__:
    sys.exit(main())
