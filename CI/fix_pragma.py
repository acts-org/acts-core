#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import argparse
import os
from glob import glob
import re

code_format = """
#pragma once
{code}
""".strip()


def fix_pragma(file):
    with open(file, "r+") as f:
        text = f.read().strip()

        def repl(m):
            code = m.group(2).strip()
            return code_format.format(code=code)

        newtext, num = re.subn(
            r"#ifndef (.*)\n#define \1.*\n((:?.|\n)+)#endif.*", repl, text, 1
        )
        if num == 1:
            f.seek(0)
            f.truncate()
            f.write(newtext)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("input")
    args = p.parse_args()

    headers = []

    if os.path.isfile(args.input):
        headers = [args.input]
    elif os.path.isdir(args.input):
        patterns = ["**/*.hpp", "**/*.h"]
        headers = sum(
            [glob(os.path.join(args.input, p), recursive=True) for p in patterns], []
        )
    else:
        headers = glob(args.input, recursive=True)

    # for h in headers: print(h)

    for h in headers:
        fix_pragma(h)


if "__main__" == __name__:
    main()
