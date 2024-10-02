#!/usr/bin/env python3

from pathlib import Path
import os
import argparse
from fnmatch import fnmatch
import re
import sys


type_list = [
    "size_t",
    "ptrdiff_t",
    "nullptr_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "max_align_t",
]

github = "GITHUB_ACTIONS" in os.environ


def handle_file(file: Path, fix: bool, c_type: str) -> list[tuple[int, str]]:
    ex = re.compile(rf"\b(?<!std::){c_type}\b")

    content = file.read_text()
    lines = content.splitlines()

    changed_lines = []

    for i, oline in enumerate(lines):
        line, n_subs = ex.subn(rf"std::{c_type}", oline)
        lines[i] = line
        if n_subs > 0:
            changed_lines.append((i, oline))

    if fix and len(changed_lines) > 0:
        file.write_text("\n".join(lines) + "\n")

    return changed_lines


def main():
    p = argparse.ArgumentParser()
    p.add_argument("input", nargs="+")
    p.add_argument("--fix", action="store_true", help="Attempt to fix C-style types.")
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
        for c_type in type_list:
            changed_lines = handle_file(file=filepath, fix=args.fix, c_type=c_type)
            if len(changed_lines) > 0:
                exit_code = 1
                print()
                print(filepath)
                for i, oline in changed_lines:
                    print(f"{i}: {oline}")

                    if github:
                        print(
                            f"::error file={filepath},line={i+1},title=Do not use C-style {c_type}::Replace {c_type} with std::{c_type}"
                        )

    return exit_code


if "__main__" == __name__:
    sys.exit(main())
