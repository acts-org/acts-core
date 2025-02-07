#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import acts
from acts.examples import Sequencer
from acts.examples.simulation import addParticleGun, EtaConfig, ParticleConfig


def runParticleGun(outputDir, s=None):
    s = s or Sequencer(events=10, numThreads=-1)
    s.config.logLevel = acts.logging.INFO
    outputDir = Path(outputDir)
    addParticleGun(
        s,
        EtaConfig(-4.0, 4.0),
        ParticleConfig(2),
        outputDirCsv=outputDir / "csv",
        outputDirRoot=outputDir,
        printParticles=True,
    )
    return s


if "__main__" == __name__:
    runParticleGun(Path.cwd()).run()
