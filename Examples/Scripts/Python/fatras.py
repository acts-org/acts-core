#!/usr/bin/env python3
from pathlib import Path

import acts
import acts.examples
from acts.examples.simulation import addParticleGun, addFatras, EtaConfig

u = acts.UnitConstants


def runFatras(trackingGeometry, field, outputDir, s: acts.examples.Sequencer = None):
    s = s or acts.examples.Sequencer(events=100, numThreads=-1)
    s.config.logLevel = acts.logging.INFO
    rnd = acts.examples.RandomNumbers()
    s = addParticleGun(
        s,
        EtaConfig(-2.0, 2.0),
        rnd=rnd,
    )
    outputDir = Path(outputDir)
    return addFatras(
        s,
        trackingGeometry,
        field,
        outputDirCsv=outputDir / "csv",
        outputDirRoot=outputDir,
        rnd=rnd,
    )


if "__main__" == __name__:
    gdc = acts.examples.GenericDetector.Config()
    detector = acts.examples.GenericDetector()
    trackingGeometry, contextDecorators = detector.finalize(gdc, None)

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    runFatras(trackingGeometry, field, Path.cwd()).run()
