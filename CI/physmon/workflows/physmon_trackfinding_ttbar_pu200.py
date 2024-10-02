#!/usr/bin/env python3

import tempfile
from pathlib import Path
import shutil

import acts
from acts.examples.simulation import (
    addPythia8,
    addFatras,
    addDigitization,
    ParticleSelectorConfig,
)
from acts.examples.reconstruction import (
    addSeeding,
    TruthSeedRanges,
    SeedFinderConfigArg,
    SeedFinderOptionsArg,
    SeedingAlgorithm,
    CkfConfig,
    addCKFTracks,
    addAmbiguityResolution,
    AmbiguityResolutionConfig,
    addVertexFitting,
    VertexFinder,
    TrackSelectorConfig,
)

from physmon_common import makeSetup

u = acts.UnitConstants

setup = makeSetup()


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

    addFatras(
        s,
        setup.trackingGeometry,
        setup.field,
        rnd=rnd,
        preSelectParticles=ParticleSelectorConfig(
            rho=(0.0, 24 * u.mm),
            absZ=(0.0, 1.0 * u.m),
        ),
    )

    addDigitization(
        s,
        setup.trackingGeometry,
        setup.field,
        digiConfigFile=setup.digiConfig,
        rnd=rnd,
    )

    addSeeding(
        s,
        setup.trackingGeometry,
        setup.field,
        TruthSeedRanges(pt=(500.0 * u.MeV, None), nHits=(9, None)),
        SeedFinderConfigArg(
            r=(33 * u.mm, 200 * u.mm),
            deltaR=(1 * u.mm, 60 * u.mm),
            collisionRegion=(-250 * u.mm, 250 * u.mm),
            z=(-2000 * u.mm, 2000 * u.mm),
            maxSeedsPerSpM=1,
            sigmaScattering=5,
            radLengthPerSeed=0.1,
            minPt=500 * u.MeV,
            impactMax=3 * u.mm,
        ),
        SeedFinderOptionsArg(bFieldInZ=2 * u.T, beamPos=(0.0, 0.0)),
        seedingAlgorithm=SeedingAlgorithm.Default,
        initialSigmas=[
            1 * u.mm,
            1 * u.mm,
            1 * u.degree,
            1 * u.degree,
            0.1 * u.e / u.GeV,
            1 * u.ns,
        ],
        initialSigmaPtRel=0.01,
        initialVarInflation=[1.0] * 6,
        geoSelectionConfigFile=setup.geoSel,
        outputDirRoot=tp,
    )

    addCKFTracks(
        s,
        setup.trackingGeometry,
        setup.field,
        TrackSelectorConfig(
            pt=(500 * u.MeV, None),
            loc0=(-4.0 * u.mm, 4.0 * u.mm),
            nMeasurementsMin=6,
            maxHoles=2,
            maxOutliers=2,
        ),
        CkfConfig(
            chi2CutOffMeasurement=15.0,
            chi2CutOffOutlier=25.0,
            numMeasurementsCutOff=10,
            seedDeduplication=True,
            stayOnSeed=True,
        ),
        outputDirRoot=tp,
    )

    addAmbiguityResolution(
        s,
        AmbiguityResolutionConfig(
            maximumSharedHits=3,
            maximumIterations=100000,
            nMeasurementsMin=6,
        ),
        outputDirRoot=tp,
    )

    s.addAlgorithm(
        acts.examples.TracksToParameters(
            level=acts.logging.INFO,
            inputTracks="tracks",
            outputTrackParameters="trackParameters",
        )
    )

    addVertexFitting(
        s,
        setup.field,
        tracks="tracks",
        trackParameters="trackParameters",
        outputProtoVertices="amvf_gauss_notime_protovertices",
        outputVertices="amvf_gauss_notime_fittedVertices",
        seeder=acts.VertexSeedFinder.GaussianSeeder,
        vertexFinder=VertexFinder.AMVF,
        outputDirRoot=tp / "amvf_gauss_notime",
    )

    addVertexFitting(
        s,
        setup.field,
        tracks="tracks",
        trackParameters="trackParameters",
        outputProtoVertices="amvf_grid_time_protovertices",
        outputVertices="amvf_grid_time_fittedVertices",
        seeder=acts.VertexSeedFinder.AdaptiveGridSeeder,
        useTime=True,
        vertexFinder=VertexFinder.AMVF,
        outputDirRoot=tp / "amvf_grid_time",
    )

    s.run()

    shutil.move(
        tp / "performance_ambi.root",
        tp / "performance_ckf_ambi.root",
    )
    for vertexing in ["amvf_gauss_notime", "amvf_grid_time"]:
        shutil.move(
            tp / f"{vertexing}/performance_vertexing.root",
            tp / f"performance_vertexing_{vertexing}.root",
        )

    for file in [
        "performance_seeding.root",
        "tracksummary_ckf.root",
        "performance_ckf.root",
        "performance_ckf_ambi.root",
        "performance_vertexing_amvf_gauss_notime.root",
        "performance_vertexing_amvf_grid_time.root",
    ]:
        perf_file = tp / file
        assert perf_file.exists(), f"Performance file not found {perf_file}"
        shutil.copy(perf_file, setup.outdir / file)
