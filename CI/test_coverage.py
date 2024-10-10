#!/usr/bin/env python
import sys
import os
import subprocess
import argparse
import multiprocessing as mp
import re


if not os.path.exists("CMakeCache.txt"):
    print("Not in CMake build dir. Not executing")
    sys.exit(1)


def check_output(*args, **kwargs):
    p = subprocess.Popen(
        *args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, **kwargs
    )
    p.wait()
    stdout, stderr = p.communicate()
    stdout = stdout.decode("utf-8")
    return (p.returncode, stdout.strip())


# call helper function
def call(cmd):
    print(" ".join(cmd))
    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError as e:
        print("Failed, output: ", e.output)
        raise e


p = argparse.ArgumentParser()
p.add_argument("--gcov", default=check_output(["which", "gcov"])[1])
args = p.parse_args()

ret, gcovr_exe = check_output(["which", "gcovr"])
assert ret == 0, "gcovr not installed. Use 'pip install gcovr'."

ret, gcovr_version_text = check_output(["gcovr", "--version"])
gcovr_version = tuple(
    map(int, re.match(r"gcovr (\d+\.\d+)", gcovr_version_text).group(1).split("."))
)

extra_flags = []

print(f"Found gcovr version {gcovr_version[0]}.{gcovr_version[1]}")
if gcovr_version < (5,):
    print("Consider upgrading to a newer gcovr version.")
elif gcovr_version == (5, 1):
    assert False and "Version 5.1 does not support parallel processing of gcov data"
elif gcovr_version >= (6,):
    extra_flags += ["--exclude-noncode-lines"]

gcovr = [gcovr_exe]

script_dir = os.path.dirname(__file__)
source_dir = os.path.abspath(os.path.join(script_dir, ".."))
coverage_dir = os.path.abspath("coverage")

if not os.path.exists(coverage_dir):
    os.makedirs(coverage_dir)

excludes = ["-e", "../Tests/", "-e", r".*json\.hpp"]

# create the html report
call(
    gcovr
    + ["-r", source_dir]
    + ["--gcov-executable", args.gcov]
    + ["-j", str(mp.cpu_count())]
    + excludes
    + extra_flags
    + ["--sonarqube", "coverage/cov.xml"]
)

call(
    gcovr
    + ["-r", source_dir]
    + ["-j", str(mp.cpu_count())]
    + ["--gcov-executable", args.gcov]
    + excludes
    + extra_flags
)
