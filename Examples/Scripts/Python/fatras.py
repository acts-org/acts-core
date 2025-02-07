#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import acts
import acts.examples
from acts.examples.simulation import addParticleGun, addFatras, EtaConfig

u = acts.UnitConstants


def runFatras(trackingGeometry, field, outputDir, s: acts.examples.Sequencer = None):
    s = s or acts.examples.Sequencer(events=100, numThreads=-1)
    s.config.logLevel = acts.logging.INFO
    rnd = acts.examples.RandomNumbers()
    addParticleGun(
        s,
        EtaConfig(-2.0, 2.0),
        rnd=rnd,
    )
    outputDir = Path(outputDir)
    addFatras(
        s,
        trackingGeometry,
        field,
        outputDirCsv=outputDir / "csv",
        outputDirRoot=outputDir,
        rnd=rnd,
    )
    return s


if "__main__" == __name__:
    gdc = acts.examples.GenericDetector.Config()
    gd = acts.examples.GenericDetector(gdc)
    trackingGeometry, decorators, _ = gd.trackingGeometry()

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    runFatras(trackingGeometry, field, Path.cwd()).run()
