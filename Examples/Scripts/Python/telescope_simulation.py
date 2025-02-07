#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import acts
import acts.examples
from acts.examples.simulation import (
    addParticleGun,
    EtaConfig,
    PhiConfig,
    ParticleConfig,
    addFatras,
    addGeant4,
)

u = acts.UnitConstants

if "__main__" == __name__:
    detector = acts.examples.TelescopeDetector(
        bounds=[200, 200],
        positions=[30, 60, 90, 120, 150, 180, 210, 240, 270],
        stereos=[0, 0, 0, 0, 0, 0, 0, 0, 0],
        binValue=2,
    )
    trackingGeometry = detector.trackingGeometry()

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    outputDir = Path.cwd() / "telescope_simulation"
    if not outputDir.exists():
        outputDir.mkdir()

    for geant, postfix in [(False, "fatras"), (True, "geant4")]:
        rnd = acts.examples.RandomNumbers(seed=42)

        s = acts.examples.Sequencer(events=1, numThreads=1, logLevel=acts.logging.INFO)

        addParticleGun(
            s,
            EtaConfig(-10.0, 10.0),
            PhiConfig(0.0, 360.0 * u.degree),
            ParticleConfig(1000, acts.PdgParticle.eMuon, False),
            multiplicity=1,
            rnd=rnd,
            outputDirRoot=outputDir / postfix,
        )

        if geant:
            addGeant4(
                s,
                detector,
                trackingGeometry,
                field,
                rnd=rnd,
                outputDirRoot=outputDir / postfix,
                outputDirCsv=outputDir / postfix,
                logLevel=acts.logging.VERBOSE,
            )
        else:
            addFatras(
                s,
                trackingGeometry,
                field,
                rnd=rnd,
                outputDirRoot=outputDir / postfix,
            )

        s.run()
