#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import os
import sys
import argparse
from subprocess import check_output


def main():
    p = argparse.ArgumentParser()
    p.add_argument("input")
    p.add_argument("--exclude", nargs="+")
    p.add_argument("--fix", action="store_true")
    p.add_argument("--reject-multiple-newlines", action="store_true")
    p.add_argument("--github", action="store_true")
    args = p.parse_args()

    files = (
        str(
            check_output(
                [
                    "find",
                    args.input,
                    "-iname",
                    "*.cpp",
                    "-or",
                    "-iname",
                    "*.hpp",
                    "-or",
                    "-iname",
                    "*.ipp",
                ]
                + sum((["-not", "-path", exclude] for exclude in args.exclude), [])
            ),
            "utf-8",
        )
        .strip()
        .split("\n")
    )

    failed = []

    for file in files:
        file = os.path.normpath(file)

        with open(file) as f:
            lines = f.readlines()

        if not lines[-1].endswith("\n"):
            print(f"Missing newline at end of file: {file}")
            if args.fix:
                with open(file, "a") as f:
                    f.write("\n")
            else:
                failed.append(file)
            if args.github:
                print(
                    f"::error file={file},line={len(lines)},title=End of file check::missing newline"
                )
        elif args.reject_multiple_newlines and lines[-1] == "\n":
            print(f"Multiple newlines at end of file: {file}")
            if args.fix:
                while lines[-1] == "\n":
                    lines.pop(-1)
                with open(file, "w") as f:
                    f.write("".join(lines))
            else:
                failed.append(file)
            if args.github:
                print(
                    f"::error file={{{file}}},line={{{len(lines)}}},title=End of file check::multiple newlines"
                )

    if failed:
        print(f"failed for files: {' '.join(failed)}")
        return 1

    print("success")
    return 0


if "__main__" == __name__:
    sys.exit(main())
