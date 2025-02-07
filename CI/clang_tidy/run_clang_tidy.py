#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
from multiprocessing import cpu_count
import subprocess
from subprocess import check_call, check_output, CalledProcessError
from pathlib import Path
import re
import sys
import os
import threading

import rich.console
import rich.progress
import rich.panel
import rich.live
import rich.text
import rich.table
import rich.rule
import rich.spinner


def which(cmd: str):
    try:
        return check_output(["command", "-v", cmd]).decode().strip()
    except CalledProcessError:
        return None


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--clang-tidy", default=which("clang-tidy"))
    p.add_argument("--clang-format", default=which("clang-format"))
    p.add_argument("--jobs", "-j", type=int, default=cpu_count())
    p.add_argument("--fix", action="store_true")
    p.add_argument("--include", action="append", default=[])
    p.add_argument("--exclude", action="append", default=[])
    p.add_argument("--ignore-compiler-errors", action="store_true")
    p.add_argument("build", type=Path)
    p.add_argument("source", type=Path)

    args = p.parse_args()

    assert args.clang_tidy is not None, "clang-tidy not found"
    assert args.clang_format is not None, "clang-format not found"

    args.include = [re.compile(f) for f in args.include]
    args.exclude = [re.compile(f) for f in args.exclude]

    check_call([args.clang_tidy, "--version"], stdout=subprocess.DEVNULL)
    check_call([args.clang_format, "--version"], stdout=subprocess.DEVNULL)

    assert (
        args.build.exists() and args.build.is_dir()
    ), f"{args.build} is not a directory"
    assert (
        args.source.exists() and args.source.is_dir()
    ), f"{args.source} is not a directory"

    futures = []
    files = []
    active_files = {}
    active_file_lock = threading.Lock()

    def run(file: Path):
        with active_file_lock:
            active_files[threading.current_thread().ident] = file
        cmd = [args.clang_tidy, "-p", args.build, file]
        if args.fix:
            cmd.append("-fix")

        try:
            out = check_output(cmd, stderr=subprocess.STDOUT).decode().strip()
            error = False
        except CalledProcessError as e:
            out = e.output.decode().strip()
            if args.ignore_compiler_errors and "Found compiler error(s)." in out:
                out = "Found compiler error(s)."
                error = False
            else:
                error = True
        finally:
            with active_file_lock:
                active_files[threading.current_thread().ident] = None
        return file, out, error

    for dirpath, _, filenames in os.walk(args.source):
        dirpath = Path(dirpath)
        for file in filenames:
            file = dirpath / file
            if (
                file.suffix in (".hpp", ".cpp", ".ipp")
                and (
                    len(args.include) == 0
                    or any(flt.match(str(file)) for flt in args.include)
                )
                and not any(flt.match(str(file)) for flt in args.exclude)
            ):
                files.append(file)

    with ThreadPoolExecutor(args.jobs) as tp:
        for file in files:
            assert file.exists(), f"{file} does not exist"
            futures.append(tp.submit(run, file))

        error = False

        console = rich.console.Console()

        prog = rich.progress.Progress()
        log = []

        def make_display():
            t = rich.table.Table.grid()
            t.add_column()
            t.add_column()
            with active_file_lock:
                for f in active_files.values():
                    if f is None:
                        t.add_row("")
                    else:
                        t.add_row(rich.spinner.Spinner("dots", style="green"), f" {f}")

            ot = rich.table.Table.grid(expand=True)
            ot.add_column(ratio=1)
            ot.add_column(ratio=1)

            def emoji(err):
                return ":red_circle:" if err else ":green_circle:"

            ot.add_row(
                t,
                rich.console.Group(
                    *[f"{emoji(err)} {line}" for err, line in log[-args.jobs :]]
                ),
            )

            return rich.console.Group(rich.rule.Rule(), ot, prog)

        task = prog.add_task("Running clang-tidy", total=len(futures))

        with rich.live.Live(
            make_display(), console=console, refresh_per_second=20, transient=False
        ) as live:
            for f in as_completed(futures):
                file, result, this_error = f.result()
                log.append((this_error, file))
                error = this_error or error
                console.print(
                    rich.panel.Panel(
                        result, title=str(file), style="red" if this_error else ""
                    )
                )
                prog.advance(task)
                live.update(make_display())
            live.refresh()

        if error:
            return 1
        else:
            return 0


if __name__ == "__main__":
    sys.exit(main())
