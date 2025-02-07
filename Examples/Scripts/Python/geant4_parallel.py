#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path
from multiprocessing import Pool
from functools import partial

# This script runs a Geant4 simulation in parallel by passing chunks of events to subprocesses.
# This is a workaround to achieve parallel processing even though Geant4 is not thread-save
# and thus the internal parallelism of the ACTS examples framework cannot be used.
#
# Note that:
# ==========
#
# * This should give equivalent results to a sequential run if the RNG is initialized with
#   the same seed in all runs
#
# * So far this works only for csv outputs, since they write the results in one file per event
#   (the naming of the csv-files should be equivalent to a sequential run)
#
# * In principle it is not difficult to extend this for ROOT files as well. One would need to
#   write the root-files into separate directory per chunk, and then use ROOT's hadd to combine
#   the output files.
#


def runGeant4EventRange(detector, trackingGeometry, beginEvent, endEvent, outputDir):
    import acts
    import acts.examples
    from acts.examples.simulation import addParticleGun, addGeant4, EtaConfig

    u = acts.UnitConstants

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))
    rnd = acts.examples.RandomNumbers(seed=42)

    s = acts.examples.Sequencer(
        events=endEvent - beginEvent, skip=beginEvent, numThreads=1
    )

    outputDir = Path(outputDir)
    addParticleGun(
        s,
        EtaConfig(-2.0, 2.0),
        rnd=rnd,
        outputDirCsv=outputDir / "csv",
        outputDirRoot=None,
    )
    addGeant4(
        s,
        detector,
        trackingGeometry,
        field,
        outputDirCsv=outputDir / "csv",
        outputDirRoot=None,
        rnd=rnd,
    )

    s.run()


if "__main__" == __name__:
    from acts.examples.odd import getOpenDataDetector

    detector = getOpenDataDetector()
    trackingGeometry = detector.trackingGeometry()
    decorators = detector.contextDecorators()

    n_events = 100
    n_jobs = 8

    chunksize = n_events // (n_jobs - 1)
    begins = range(0, n_events, chunksize)
    ends = [min(b + chunksize, n_events) for b in begins]

    outputDir = Path.cwd()

    with Pool(n_jobs) as p:
        p.starmap(partial(runGeant4EventRange, outputDir=outputDir), zip(begins, ends))
