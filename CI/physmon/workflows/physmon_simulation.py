#!/usr/bin/env python3

import tempfile
from pathlib import Path
import shutil

import acts
from acts.examples.simulation import (
    addFatras,
    addGeant4,
    ParticleSelectorConfig,
    addPythia8,
)

from physmon_common import makeSetup

u = acts.UnitConstants

setup = makeSetup()


with tempfile.TemporaryDirectory() as temp:
    tp = Path(temp)

    rnd = acts.examples.RandomNumbers(seed=42)

    s = acts.examples.Sequencer(
        events=1000,
        numThreads=1,
        logLevel=acts.logging.INFO,
    )

    for d in setup.decorators:
        s.addContextDecorator(d)

    s.addReader(
        acts.examples.EventGenerator(
            level=acts.logging.INFO,
            generators=[
                acts.examples.EventGenerator.Generator(
                    multiplicity=acts.examples.FixedMultiplicityGenerator(n=1),
                    vertex=acts.examples.GaussianVertexGenerator(
                        mean=acts.Vector4(0, 0, 0, 0),
                        stddev=acts.Vector4(10 * u.um, 10 * u.um, 50 * u.mm, 1 * u.ns),
                    ),
                    particles=acts.examples.ParametricParticleGenerator(
                        p=(1 * u.GeV, 100 * u.GeV),
                        pTransverse=True,
                        eta=(-3.0, 3.0),
                        phi=(0.0 * u.degree, 360.0 * u.degree),
                        pdg=pdg,
                        randomizeCharge=True,
                    ),
                )
                for pdg in [
                    acts.PdgParticle.eMuon,
                    acts.PdgParticle.ePionPlus,
                    acts.PdgParticle.eElectron,
                ]
            ],
            outputParticles="particles_input",
            outputVertices="vertices_input",
            randomNumbers=rnd,
        )
    )

    addFatras(
        s,
        setup.trackingGeometry,
        setup.field,
        rnd,
        enableInteractions=True,
        preSelectParticles=None,
        postSelectParticles=ParticleSelectorConfig(removeSecondaries=True),
        inputParticles="particles_input",
        outputParticlesInitial="particles_initial_fatras",
        outputParticlesFinal="particles_final_fatras",
        outputSimHits="simhits_fatras",
        outputDirRoot=tp / "fatras",
    )

    addGeant4(
        s,
        setup.detector,
        setup.trackingGeometry,
        setup.field,
        rnd,
        preSelectParticles=None,
        postSelectParticles=ParticleSelectorConfig(removeSecondaries=True),
        killVolume=setup.trackingGeometry.highestTrackingVolume,
        killAfterTime=25 * u.ns,
        killSecondaries=True,
        inputParticles="particles_input",
        outputParticlesInitial="particles_initial_geant4",
        outputParticlesFinal="particles_final_geant4",
        outputSimHits="simhits_geant4",
        outputDirRoot=tp / "geant4",
    )

    s.run()

    for file, name in [
        (tp / "fatras" / "particles_simulation.root", "particles_fatras.root"),
        (tp / "geant4" / "particles_simulation.root", "particles_geant4.root"),
    ]:
        assert file.exists(), "file not found"
        shutil.copy(file, setup.outdir / name)

with tempfile.TemporaryDirectory() as temp:
    s = acts.examples.Sequencer(
        events=3,
        numThreads=-1,
        logLevel=acts.logging.INFO,
    )

    tp = Path(temp)

    for d in setup.decorators:
        s.addContextDecorator(d)

    rnd = acts.examples.RandomNumbers(seed=42)

    addPythia8(
        s,
        hardProcess=["Top:qqbar2ttbar=on"],
        npileup=200,
        vtxGen=acts.examples.GaussianVertexGenerator(
            mean=acts.Vector4(0, 0, 0, 0),
            stddev=acts.Vector4(0.0125 * u.mm, 0.0125 * u.mm, 55.5 * u.mm, 5.0 * u.ns),
        ),
        rnd=rnd,
        outputDirRoot=tp,
    )

    s.run()

    for file, name in [
        (tp / "pythia8_particles.root", "particles_ttbar.root"),
        (tp / "pythia8_vertices.root", "vertices_ttbar.root"),
    ]:
        assert file.exists(), "file not found"
        shutil.copy(file, setup.outdir / name)
