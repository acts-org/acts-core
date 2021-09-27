#!/usr/bin/env python3
import os
import warnings

import acts
from acts.examples import (
    FixedVertexGenerator,
    ParametricParticleGenerator,
    FixedMultiplicityGenerator,
    EventGenerator,
    RandomNumbers,
)

import acts.examples.dd4hep
import acts.examples.geant4
import acts.examples.geant4.dd4hep

u = acts.UnitConstants

_geantino_recording_executed = False


def runGeantinoRecording(g4geo, outputDir, tracksPerEvent=10000, s=None):
    global _geantino_recording_executed
    if _geantino_recording_executed:
        warnings.warn("Geantino recording already ran in this process. Expect crashes")
    _geantino_recording_executed = True

    rnd = RandomNumbers(seed=228)

    s = s or acts.examples.Sequencer(events=10, numThreads=1)

    evGen = EventGenerator(
        level=acts.logging.INFO,
        generators=[
            EventGenerator.Generator(
                multiplicity=FixedMultiplicityGenerator(n=1),
                vertex=FixedVertexGenerator(stddev=acts.Vector4(0, 0, 0, 0)),
                particles=ParametricParticleGenerator(
                    p=(1 * u.GeV, 10 * u.GeV), eta=(-4, 4), numParticles=2
                ),
            )
        ],
        outputParticles="particles_initial",
        randomNumbers=rnd,
    )

    s.addReader(evGen)

    g4AlgCfg = acts.examples.geant4.Geant4Simulation.Config()
    g4AlgCfg.detectorConstruction = g4geo
    g4AlgCfg.tracksPerEvent = tracksPerEvent

    g4Alg = acts.examples.geant4.Geant4Simulation(
        level=acts.logging.INFO, config=g4AlgCfg
    )

    s.addAlgorithm(g4Alg)

    s.addWriter(
        acts.examples.RootMaterialTrackWriter(
            prePostStep=True,
            recalculateTotals=True,
            collection=g4Alg.config.outputMaterialTracks,
            filePath=outputDir + "/" + g4Alg.config.outputMaterialTracks + ".root",
            level=acts.logging.INFO,
        )
    )

    return s


if "__main__" == __name__:
    dd4hepSvc = acts.examples.dd4hep.DD4hepGeometryService(
        xmlFileNames=["thirdparty/OpenDataDetector/xml/OpenDataDetector.xml"]
    )
    g4geo = acts.examples.geant4.dd4hep.DDG4DetectorConstruction(dd4hepSvc)

    runGeantinoRecording(g4geo=g4geo, outputDir=os.getcwd()).run()
