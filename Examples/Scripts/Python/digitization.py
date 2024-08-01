#!/usr/bin/env python3
from pathlib import Path
from typing import Optional

import acts
import acts.examples


u = acts.UnitConstants


def runDigitization(
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    outputDir: Path,
    digiConfigFile: Path,
    particlesInput: Optional[Path] = None,
    outputRoot: bool = True,
    outputCsv: bool = True,
    s: Optional[acts.examples.Sequencer] = None,
    doMerge: Optional[bool] = None,
) -> acts.examples.Sequencer:
    from acts.examples.simulation import (
        addParticleGun,
        EtaConfig,
        PhiConfig,
        ParticleConfig,
        addFatras,
        addDigitization,
    )

    s = s or acts.examples.Sequencer(
        events=1000, numThreads=-1, logLevel=acts.logging.INFO
    )
    rnd = acts.examples.RandomNumbers(seed=42)

    if particlesInput is None:
        addParticleGun(
            s,
            EtaConfig(-3.0, 3.0),
            ParticleConfig(4, acts.PdgParticle.eMuon, True),
            PhiConfig(0.0, 360.0 * u.degree),
            multiplicity=2,
            rnd=rnd,
        )
    else:
        # Read input from input collection (e.g. Pythia8 output)
        evGen = acts.examples.RootParticleReader(
            level=s.config.logLevel,
            filePath=str(particlesInput),
            outputParticles="particles_input",
        )
        s.addReader(evGen)

    outputDir = Path(outputDir)
    addFatras(
        s,
        trackingGeometry,
        field,
        rnd=rnd,
    )

    addDigitization(
        s,
        trackingGeometry,
        field,
        digiConfigFile=digiConfigFile,
        outputDirCsv=outputDir / "csv" if outputCsv else None,
        outputDirRoot=outputDir if outputRoot else None,
        rnd=rnd,
        doMerge=doMerge,
        logLevel=acts.logging.INFO,
    )

    s.addWriter(
        acts.examples.CsvTrackingGeometryWriter(
            level=acts.logging.INFO,
            trackingGeometry=trackingGeometry,
            outputDir=outputDir,
            writePerEvent=False,
        )
    )

    return s


if "__main__" == __name__:
    digi_share_dir = (
        Path(__file__).resolve().parent.parent.parent.parent
        / "Examples/Algorithms/Digitization/share"
    )

    if False:
        detector, trackingGeometry, _ = acts.examples.GenericDetector.create()
        digiConfigFile = digi_share_dir / "default-smearing-config-generic.json"
    else:
        from acts.examples.odd import getOpenDataDetector

        detector, trackingGeometry, _ = getOpenDataDetector()
        digiConfigFile = digi_share_dir / "odd-digi-geometric-config.json"

    assert digiConfigFile.exists()

    field = acts.ConstantBField(acts.Vector3(0, 0, 2 * u.T))

    runDigitization(
        trackingGeometry, field, outputDir=Path.cwd(), digiConfigFile=digiConfigFile
    ).run()
