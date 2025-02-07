#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import tempfile
from pathlib import Path
import shutil

import acts
from truth_tracking_gsf import runTruthTrackingGsf

from physmon_common import makeSetup

setup = makeSetup()

with tempfile.TemporaryDirectory() as temp:
    s = acts.examples.Sequencer(
        events=10000,
        numThreads=-1,
        logLevel=acts.logging.INFO,
    )

    tp = Path(temp)
    runTruthTrackingGsf(
        trackingGeometry=setup.trackingGeometry,
        field=setup.field,
        digiConfigFile=setup.digiConfig,
        outputDir=tp,
        s=s,
    )

    s.run()

    perf_file = tp / "performance_gsf.root"
    assert perf_file.exists(), "Performance file not found"
    shutil.copy(perf_file, setup.outdir / "performance_trackfitting.root")
