#!/usr/bin/env python3
import argparse
import os
from glob import glob
import re
from fnmatch import fnmatch
import sys


def line_fmt(line):
    return "{: >4d} ".format(line)


def code_print(code, start, maxlines=15):
    lines = code.split("\n")
    nlines = len(lines)

    lines = [line_fmt(i + start) + l for i, l in enumerate(lines)]

    nlup = int(maxlines / 2)
    nllo = maxlines - nlup - 1

    if nlines > maxlines:
        lines = lines[:nlup] + [" " * 5 + "// ..."] + lines[-nllo:]

    return "\n".join(lines)


def check_include_guards(file):
    with open(file) as f:
        text = f.read()

    match_local = list(
        re.finditer(
            r"(#ifndef [A-Za-z0-9_]*\n#define [A-Za-z0-9_]*.*)\n((:?.|\n)+?)#endif",
            text,
        )
    )
    match_global = re.search(
        r"#ifndef (.*)\n#define \1.*\n[\s\S]+#endif[A-Za-z0-9\-_/* ]*$", text
    )

    valid_global = True
    valid_local = True
    errbuf = ""

    if match_global is not None and len(match_local) <= 1:
        valid_global = False

        errbuf += "This looks like a file-spanning include guard\n"
        errbuf += "This is discouraged as per [ACTS-450]"
        errbuf += "(https://its.cern.ch/jira/browse/ACTS-450)" + "\n" * 2

        start = text[: match_global.start()].count("\n") + 1
        errbuf += code_print(match_global.group(0), start)
        errbuf += "\n" * 2

    if valid_global or len(match_local) > 1:
        for m in match_local:
            lineno = text[: m.start()].count("\n") + 1

            valid_local = False
            errbuf += "This looks like a local #ifndef / include-guard\n"
            errbuf += "This is discouraged as per [ACTS-450]"
            errbuf += "(https://its.cern.ch/jira/browse/ACTS-450)" + "\n" * 2
            errbuf += code_print(m.group(0), lineno)
            errbuf += "\n" * 2

    return valid_local, valid_global, errbuf


def main():
    p = argparse.ArgumentParser()

    input_help = """
Input files: either file path, dir path (will glob for headers) or custom glob pattern
    """
    p.add_argument("input", nargs="+", help=input_help.strip())
    p.add_argument(
        "--fail-local", "-l", action="store_true", help="Fail on local include guards"
    )
    p.add_argument(
        "--fail-global", "-g", action="store_true", help="Fail on global include guards"
    )
    p.add_argument("--quiet-local", "-ql", action="store_true")
    p.add_argument("--quiet-global", "-qg", action="store_true")
    p.add_argument("--exclude", "-e", action="append", default=[])

    args = p.parse_args()

    headers = []

    if len(args.input) == 1:
        if os.path.isdir(args.input[0]):
            patterns = ["**/*.hpp", "**/*.h"]
            headers = sum(
                [
                    glob(os.path.join(args.input[0], p), recursive=True)
                    for p in patterns
                ],
                [],
            )
        else:
            headers = glob(args.input[0], recursive=True)
    elif all([os.path.isfile(i) for i in args.input]):
        headers = args.input

    valid = True
    nlocal = 0
    nglobal = 0

    for h in headers:
        if any([fnmatch(h, e) for e in args.exclude]):
            continue
        valid_local, valid_global, errbuf = check_include_guards(h)

        if not valid_local:
            nlocal += 1
            if args.fail_local:
                valid = False
        if not valid_global:
            nglobal += 1
            if args.fail_global:
                valid = False

        if not valid_local or not valid_global:
            head = "Issue(s) in file {}:\n".format(h)
            print("-" * len(head))
            print(head)
            print(errbuf)
            print("\n")

    print("=" * 40)
    print("Checked {} files".format(len(headers)))
    print("Issues found in {} files".format(nlocal + nglobal))
    print("{} files have local include guards".format(nlocal))
    print("{} files have global include guards".format(nglobal))

    if valid:
        sys.exit(0)
    else:
        sys.exit(1)


if "__main__" == __name__:
    main()
