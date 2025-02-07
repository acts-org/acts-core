# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import subprocess
import sys


# Cannot conveniently catch linker errors, so we launch a suprocess to
# try importing and see if it works in order to provide a useful error message
try:
    subprocess.check_call(
        [sys.executable, "-c", "from acts import ActsPythonBindingsDD4hep"]
    )
except subprocess.CalledProcessError as e:
    print("Error encountered importing DD4hep. Likely you need to set LD_LIBRARY_PATH.")
    sys.exit(1)

from acts._adapter import _patch_config
from acts import ActsPythonBindingsDD4hep

_patch_config(ActsPythonBindingsDD4hep)

from acts.ActsPythonBindingsDD4hep import *
