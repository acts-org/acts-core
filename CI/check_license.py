#!/usr/bin/env python3
import argparse
import os
import sys
from subprocess import check_output
import re
import difflib
from datetime import datetime
from fnmatch import fnmatch

EXCLUDE = []


class bcolors:
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    OKGREEN = "\033[92m"
    WARNING = "\033[93m"
    FAIL = "\033[91m"
    ENDC = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"


CROSS_SYMBOL = "\u2717"


def err(string):
    if sys.stdout.isatty():
        return bcolors.FAIL + bcolors.BOLD + string + bcolors.ENDC
    else:
        return string


def main():
    p = argparse.ArgumentParser()
    p.add_argument("input", nargs="+")
    p.add_argument(
        "--fix", action="store_true", help="Attempt to fix any license issues found."
    )
    p.add_argument("--exclude", "-e", action="append", default=EXCLUDE)

    args = p.parse_args()
    print(args.exclude)

    extensions = ["cpp", "hpp", "ipp", "cuh", "cu", "C", "h"]

    if len(args.input) == 1 and os.path.isdir(args.input[0]):
        find_command = ["find", args.input[0]]
        for ext in extensions:
            find_command.extend(["-iname", f"*.{ext}", "-or"])
        # Remove the last "-or" for a valid command
        find_command = find_command[:-1]

        srcs = (
            str(
                check_output(find_command),
                "utf-8",
            )
            .strip()
            .split("\n")
        )
        srcs = filter(lambda p: not p.startswith("./build"), srcs)
    else:
        srcs = args.input

    founding_year = 2016
    year_str = f"{founding_year}"
    year = year_str

    raw = """// This file is part of the ACTS project.
//
// Copyright (C) {year} CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/."""

    reg = (
        r"\A// This file is part of the ACTS project.\n"
        + r"//\n"
        + r"// Copyright \(C\) (?P<year>.*) CERN for the benefit of the ACTS project\n"
        + r"//\n"
        + r"// This Source Code Form is subject to the terms of the Mozilla Public\n"
        + r"// License, v\. 2\.0\. If a copy of the MPL was not distributed with this\n"
        + r"// file, You can obtain one at https://mozilla.org/MPL/2.0/.\Z"
    )

    ref = re.compile(reg, re.M)
    clean_re = re.compile(r"(\(C\)) (.*) (CERN)", re.M)

    def clean(s):
        return clean_re.sub(r"\1 XXXX \3", s)

    def get_clean_lines(s):
        return [clean(l) + "\n" for l in s.split("\n")]

    error_summary = ""

    def eprint(string):
        nonlocal error_summary
        error_summary += string + "\n"

    exit = 0
    srcs = list(srcs)
    nsrcs = len(srcs)
    step = max(int(nsrcs / 20), 1)
    # Iterate over all files
    for i, src in enumerate(srcs):
        if any([fnmatch(src, e) for e in args.exclude]):
            continue

        # Print progress
        if nsrcs > 1 and i % step == 0:
            string = f"{i}/{nsrcs} -> {i / float(nsrcs) * 100.0:.2f}%"
            if sys.stdout.isatty():
                sys.stdout.write(string + "\r")
            else:
                print(string)

        # Read the header
        with open(src, "r+") as f:
            license = ""
            for _ in range(len(raw)):
                line = f.readline()
                if not line.startswith("//"):
                    break
                license += line
            license = ("".join(license)).strip()
            m = ref.search(license)

            # License could not be found in header
            if m is None:
                eprint("Invalid / missing license in " + src + "")

                exp = [l + "\n" for l in raw.format(year=year_str).split("\n")]
                act = get_clean_lines(license)

                diff = difflib.unified_diff(exp, act)
                eprint("".join(diff))
                eprint("")

                if args.fix:
                    eprint("-> fixing file (prepend)")
                    f.seek(0)
                    file_content = f.read()
                    f.seek(0)
                    stmnt = raw.format(year=year_str)
                    f.write(stmnt + "\n\n")
                    f.write(file_content)

                exit = 1
                continue

            # We have a match, need to verify year string is right
            year_act = m.group("year")
            if year_act != year_str:
                exit = 1

                eprint(f"File: {src}")
                eprint(f"=> License should say {year_str}")
                eprint(err(f"{CROSS_SYMBOL} But says: {year_act}"))

                if args.fix:
                    eprint("-> fixing file (patch year)")

                    new_license = raw.format(year=year_str)

                    # preserve rest of file as is
                    old_license_len = len(license)
                    f.seek(old_license_len)
                    file_body = f.read()
                    f.seek(0)
                    f.truncate()

                    f.seek(0)
                    f.write(new_license)
                    f.write(file_body)

                eprint("")

    print(error_summary)

    if exit != 0 and not args.fix:
        print("License problems found. You can try running again with --fix")

    sys.exit(exit)


if "__main__" == __name__:
    main()
