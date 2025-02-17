from pathlib import Path
import os
import json
import functools
import tarfile
import urllib.request
import subprocess
import sys
import re
import collections

import pytest

from helpers import (
    geant4Enabled,
    rootEnabled,
    dd4hepEnabled,
    hepmc3Enabled,
    pythia8Enabled,
    exatrkxEnabled,
    onnxEnabled,
    AssertCollectionExistsAlg,
    failure_threshold,
)


def test_gsf_debugger(tmp_path):
    path = (
        Path(__file__).parent.parent.parent.parent
        / "Examples"
        / "Scripts"
        / "GsfDebugger"
    )
    scriptdir = (
        Path(__file__).parent.parent.parent.parent / "Examples" / "Scripts" / "Python"
    )

    gsf_script = path / "make_gsf_verbose_log.py"
    assert gsf_script.exists()

    debugger = path / "src/main.py"
    assert debugger.exists()

    env = os.environ.copy()
    env["PYTHONPATH"] = f"{scriptdir}:{env['PYTHONPATH']}"
    gsf_result = subprocess.run(
        [sys.executable, gsf_script], capture_output=True, cwd=tmp_path, env=env
    )

    logfile = tmp_path / "test.log"
    with open(logfile, "w") as f:
        f.write(gsf_result.stdout.decode("utf8"))

    assert gsf_result.returncode == 0

    debugger_result = subprocess.run(
        [sys.executable, debugger, f"--logfile={logfile}", "--nogui"], cwd=tmp_path
    )
    assert debugger_result.returncode == 0
