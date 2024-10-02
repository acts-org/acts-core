#!/usr/bin/env python3
import argparse

from pathlib import Path

import acts
import acts.examples
from acts.examples.simulation import addParticleGun, addGeant4, EtaConfig
from acts.examples.odd import getOpenDataDetector, getOpenDataDetectorDirectory

u = acts.UnitConstants


def runGeant4(
    detector,
    trackingGeometry,
    field,
    outputDir,
    materialMappings=["Silicon"],
    volumeMappings=[],
    s: acts.examples.Sequencer = None,
):
    s = s or acts.examples.Sequencer(events=100, numThreads=1)
    s.config.logLevel = acts.logging.INFO
    rnd = acts.examples.RandomNumbers()
    addParticleGun(
        s,
        EtaConfig(-2.0, 2.0),
        rnd=rnd,
    )
    outputDir = Path(outputDir)
    addGeant4(
        s,
        detector,
        trackingGeometry,
        field,
        outputDirCsv=outputDir / "csv",
        outputDirRoot=outputDir,
        rnd=rnd,
        materialMappings=materialMappings,
        volumeMappings=volumeMappings,
    )
    return s


if "__main__" == __name__:
    p = argparse.ArgumentParser()

    p.add_argument(
        "--experimental",
        action=argparse.BooleanOptionalAction,
        help="Construct experimental geometry",
    )

    args = p.parse_args()

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    if args.experimental:
        from acts.examples.dd4hep import (
            DD4hepDetector,
            DD4hepDetectorOptions,
            DD4hepGeometryService,
        )

        print(">>> Running experimental geometry <<<")
        odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"

        # Create the dd4hep geometry service and detector
        dd4hepConfig = DD4hepGeometryService.Config()
        dd4hepConfig.logLevel = acts.logging.INFO
        dd4hepConfig.xmlFileNames = [str(odd_xml)]
        dd4hepGeometryService = DD4hepGeometryService(dd4hepConfig)
        dd4hepDetector = DD4hepDetector(dd4hepGeometryService)

        cOptions = DD4hepDetectorOptions(logLevel=acts.logging.INFO, emulateToGraph="")

        # Context and options
        geoContext = acts.GeometryContext()
        [detector, contextors, store] = dd4hepDetector.finalize(geoContext, cOptions)
        runGeant4(detector, detector, field, Path.cwd()).run()
    else:
        detector, trackingGeometry, decorators = getOpenDataDetector()
        runGeant4(detector, trackingGeometry, field, Path.cwd()).run()
