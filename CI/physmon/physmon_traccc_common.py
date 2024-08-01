#!/usr/bin/env python3

import tempfile
from pathlib import Path
import shutil

import acts
from acts.examples.simulation import (
    addParticleGun,
    MomentumConfig,
    EtaConfig,
    PhiConfig,
    ParticleConfig,
    addFatras,
    addDigitization,
)

from acts.examples.reconstruction import (
    addTracccChain
)

from physmon_common import makeSetup

setup = makeSetup(useGeometricConfig=True)


def runTracccChain(platform):
    u = acts.UnitConstants
    with tempfile.TemporaryDirectory() as temp:
        s = acts.examples.Sequencer(
            events=500,
            numThreads=-1,
            logLevel=acts.logging.INFO,
            trackFpes=False,
        )

        tp = Path(temp)

        for d in setup.decorators:
            s.addContextDecorator(d)

        rnd = acts.examples.RandomNumbers(seed=42)

        addParticleGun(
            s,
            MomentumConfig(1.0 * u.GeV, 10.0 * u.GeV, transverse=True),
            EtaConfig(-3.0, 3.0),
            PhiConfig(0.0, 360.0 * u.degree),
            ParticleConfig(4, acts.PdgParticle.eMuon, randomizeCharge=True),
            vtxGen=acts.examples.GaussianVertexGenerator(
                mean=acts.Vector4(0, 0, 0, 0),
                stddev=acts.Vector4(
                    0.0125 * u.mm, 0.0125 * u.mm, 55.5 * u.mm, 1.0 * u.ns
                ),
            ),
            multiplicity=50,
            rnd=rnd,
        )

        addFatras(
            s,
            setup.trackingGeometry,
            setup.field,
            enableInteractions=True,
            rnd=rnd,
        )

        addDigitization(
            s,
            setup.trackingGeometry,
            setup.field,
            digiConfigFile=setup.digiConfig,
            rnd=rnd,
            minEnergyDeposit=0,
            # Traccc chain expects doMerge and mergeCommonCorner 
            # as ACTS and Traccc clusterization must give the 
            # same results to be able to pair the measurements
            # (which is used for truth matching).
            doMerge=True,
            mergeCommonCorner=True,
        )

        chainConfig = acts.examples.TracccChainConfig()

        addTracccChain(
            s,
            setup.trackingGeometry,
            setup.field,
            digiConfigFile=setup.digiConfig,
            inputCells="cells",
            outputDirRoot=tp,
            chainConfig=chainConfig,
            logLevel=acts.logging.INFO,
            platform=platform,
        )

        s.run()
        del s

        name = f"tracksummary_traccc_{platform}";
        perf_file = tp / f"{name}.root"
        assert perf_file.exists(), "Performance file not found"
        shutil.copy(perf_file, setup.outdir / f"{name}.root")