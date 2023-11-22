from pathlib import Path
from typing import Optional, Union, List
from enum import Enum
from collections import namedtuple
import warnings

import acts
import acts.examples

u = acts.UnitConstants

SeedingAlgorithm = Enum(
    "SeedingAlgorithm",
    "Default TruthSmeared TruthEstimated Orthogonal HoughTransform FTF",
)

TruthSeedRanges = namedtuple(
    "TruthSeedRanges",
    ["rho", "z", "phi", "eta", "absEta", "pt", "nHits"],
    defaults=[(None, None)] * 7,
)

ParticleSmearingSigmas = namedtuple(
    "ParticleSmearingSigmas",
    ["d0", "d0PtA", "d0PtB", "z0", "z0PtA", "z0PtB", "t0", "phi", "theta", "pRel"],
    defaults=[None] * 10,
)

SeedFinderConfigArg = namedtuple(
    "SeedFinderConfig",
    [
        "maxSeedsPerSpM",
        "cotThetaMax",
        "sigmaScattering",
        "radLengthPerSeed",
        "minPt",
        "impactMax",
        "deltaPhiMax",
        "interactionPointCut",
        "deltaZMax",
        "maxPtScattering",
        "zBinEdges",
        "zBinsCustomLooping",
        "skipZMiddleBinSearch",
        "rRangeMiddleSP",
        "useVariableMiddleSPRange",
        "binSizeR",
        "seedConfirmation",
        "centralSeedConfirmationRange",
        "forwardSeedConfirmationRange",
        "deltaR",  # (min,max)
        "deltaRBottomSP",  # (min,max)
        "deltaRTopSP",  # (min,max)
        "deltaRMiddleSPRange",  # (min,max)
        "collisionRegion",  # (min,max)
        "r",  # (min,max)
        "z",  # (min,max)
        "zOutermostLayers",  # (min,max)
    ],
    defaults=[None] * 19 + [(None, None)] * 8,
)
SeedFinderOptionsArg = namedtuple(
    "SeedFinderOptions", ["beamPos", "bFieldInZ"], defaults=[(None, None), None]
)

SeedFilterConfigArg = namedtuple(
    "SeedFilterConfig",
    [
        "impactWeightFactor",
        "zOriginWeightFactor",
        "compatSeedWeight",
        "compatSeedLimit",
        "numSeedIncrement",
        "seedWeightIncrement",
        "seedConfirmation",
        "maxSeedsPerSpMConf",
        "maxQualitySeedsPerSpMConf",
        "useDeltaRorTopRadius",
        "deltaRMin",
    ],
    defaults=[None] * 11,
)

SpacePointGridConfigArg = namedtuple(
    "SeedGridConfig",
    [
        "rMax",
        "zBinEdges",
        "phiBinDeflectionCoverage",
        "impactMax",
        "deltaRMax",
        "maxPhiBins",
        "phi",  # (min,max)
    ],
    defaults=[None] * 6 + [(None, None)] * 1,
)

SeedingAlgorithmConfigArg = namedtuple(
    "SeedingAlgorithmConfig",
    [
        "allowSeparateRMax",
        "zBinNeighborsTop",
        "zBinNeighborsBottom",
        "numPhiNeighbors",
    ],
    defaults=[None] * 4,
)

TruthEstimatedSeedingAlgorithmConfigArg = namedtuple(
    "TruthSeederConfig",
    [
        "deltaR",  # (min,max)
    ],
    defaults=[(None, None)],
)

TrackSelectorConfig = namedtuple(
    "TrackSelectorConfig",
    ["loc0", "loc1", "time", "eta", "absEta", "pt", "phi", "nMeasurementsMin"],
    defaults=[(None, None)] * 7 + [None],
)

CkfConfig = namedtuple(
    "CkfConfig",
    ["chi2CutOff", "numMeasurementsCutOff", "maxSteps"],
    defaults=[15.0, 10, None],
)

AmbiguityResolutionConfig = namedtuple(
    "AmbiguityResolutionConfig",
    ["maximumSharedHits", "nMeasurementsMin", "maximumIterations"],
    defaults=[None] * 3,
)

AmbiguityResolutionMLConfig = namedtuple(
    "AmbiguityResolutionMLConfig",
    ["maximumSharedHits", "nMeasurementsMin", "maximumIterations"],
    defaults=[None] * 3,
)

AmbiguityResolutionMLDBScanConfig = namedtuple(
    "AmbiguityResolutionMLDBScanConfig",
    ["nMeasurementsMin", "epsilonDBScan", "minPointsDBScan"],
    defaults=[None] * 3,
)


class VertexFinder(Enum):
    Truth = (1,)
    AMVF = (2,)
    Iterative = (3,)


@acts.examples.NamedTypeArgs(
    seedingAlgorithm=SeedingAlgorithm,
    truthSeedRanges=TruthSeedRanges,
    particleSmearingSigmas=ParticleSmearingSigmas,
    seedFinderConfigArg=SeedFinderConfigArg,
    seedFinderOptionsArg=SeedFinderOptionsArg,
    seedFilterConfigArg=SeedFilterConfigArg,
    spacePointGridConfigArg=SpacePointGridConfigArg,
    seedingAlgorithmConfigArg=SeedingAlgorithmConfigArg,
    truthEstimatedSeedingAlgorithmConfigArg=TruthEstimatedSeedingAlgorithmConfigArg,
    logLevel=acts.logging.Level,
)
def addSeeding(
    s: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    geoSelectionConfigFile: Optional[Union[Path, str]] = None,
    layerMappingConfigFile: Optional[Union[Path, str]] = None,
    fastrack_inputConfigFile: Optional[Union[Path, str]] = None,
    seedingAlgorithm: SeedingAlgorithm = SeedingAlgorithm.Default,
    truthSeedRanges: Optional[TruthSeedRanges] = TruthSeedRanges(),
    particleSmearingSigmas: ParticleSmearingSigmas = ParticleSmearingSigmas(),
    initialSigmas: Optional[list] = None,
    initialVarInflation: Optional[list] = None,
    seedFinderConfigArg: SeedFinderConfigArg = SeedFinderConfigArg(),
    seedFinderOptionsArg: SeedFinderOptionsArg = SeedFinderOptionsArg(),
    seedFilterConfigArg: SeedFilterConfigArg = SeedFilterConfigArg(),
    spacePointGridConfigArg: SpacePointGridConfigArg = SpacePointGridConfigArg(),
    seedingAlgorithmConfigArg: SeedingAlgorithmConfigArg = SeedingAlgorithmConfigArg(),
    houghTransformConfig: acts.examples.HoughTransformSeeder.Config = acts.examples.HoughTransformSeeder.Config(),
    truthEstimatedSeedingAlgorithmConfigArg: TruthEstimatedSeedingAlgorithmConfigArg = TruthEstimatedSeedingAlgorithmConfigArg(),
    particleHypothesis: Optional[
        acts.ParticleHypothesis
    ] = acts.ParticleHypothesis.pion,
    inputParticles: str = "particles",
    outputDirRoot: Optional[Union[Path, str]] = None,
    logLevel: Optional[acts.logging.Level] = None,
    rnd: Optional[acts.examples.RandomNumbers] = None,
) -> None:
    """This function steers the seeding
    Parameters
    ----------
    s: Sequencer
        the sequencer module to which we add the Seeding steps (returned from addSeeding)
    trackingGeometry : tracking geometry
    field : magnetic field
    geoSelectionConfigFile : Path|str, path, None
        Json file for space point geometry selection. Not required for SeedingAlgorithm.TruthSmeared.
    seedingAlgorithm : SeedingAlgorithm, Default
        seeding algorithm to use: one of Default (no truth information used), TruthSmeared, TruthEstimated
    truthSeedRanges : TruthSeedRanges(rho, z, phi, eta, absEta, pt, nHits)
        TruthSeedSelector configuration. Each range is specified as a tuple of (min,max).
        Defaults of no cuts specified in Examples/Algorithms/TruthTracking/ActsExamples/TruthTracking/TruthSeedSelector.hpp
        If specified as None, don't run ParticleSmearing at all (and use addCKFTracks(selectedParticles="particles"))
    particleSmearingSigmas : ParticleSmearingSigmas(d0, d0PtA, d0PtB, z0, z0PtA, z0PtB, t0, phi, theta, pRel)
        ParticleSmearing configuration.
        Defaults specified in Examples/Algorithms/TruthTracking/ActsExamples/TruthTracking/ParticleSmearing.hpp
    initialSigmas : list
        Sets the initial covariance matrix diagonal. This is ignored in case of TruthSmearing.
        Defaults specified in Examples/Algorithms/TrackFinding/include/ActsExamples/TrackFinding/TrackParamsEstimationAlgorithm.hpp
    initialVarInflation : list
        List of 6 scale factors to inflate the initial covariance matrix
        Defaults (all 1) specified in Examples/Algorithms/TruthTracking/ActsExamples/TruthTracking/ParticleSmearing.hpp
    seedFinderConfigArg : SeedFinderConfigArg(maxSeedsPerSpM, cotThetaMax, sigmaScattering, radLengthPerSeed, minPt, impactMax, deltaPhiMax, interactionPointCut, deltaZMax, maxPtScattering, zBinEdges, zBinsCustomLooping, skipZMiddleBinSearch, rRangeMiddleSP, useVariableMiddleSPRange, binSizeR, seedConfirmation, centralSeedConfirmationRange, forwardSeedConfirmationRange, deltaR, deltaRBottomSP, deltaRTopSP, deltaRMiddleSPRange, collisionRegion, r, z, zOutermostLayers)
        SeedFinderConfig settings. deltaR, deltaRBottomSP, deltaRTopSP, deltaRMiddleSPRange, collisionRegion, r, z, zOutermostLayers are ranges specified as a tuple of (min,max). beamPos is specified as (x,y).
        Defaults specified in Core/include/Acts/Seeding/SeedFinderConfig.hpp
    seedFinderOptionsArg :  SeedFinderOptionsArg(bFieldInZ, beamPos)
        Defaults specified in Core/include/Acts/Seeding/SeedFinderConfig.hpp
    seedFilterConfigArg : SeedFilterConfigArg(compatSeedWeight, compatSeedLimit, numSeedIncrement, seedWeightIncrement, seedConfirmation, maxSeedsPerSpMConf, maxQualitySeedsPerSpMConf, useDeltaRorTopRadius)
                                Defaults specified in Core/include/Acts/Seeding/SeedFilterConfig.hpp
    spacePointGridConfigArg : SpacePointGridConfigArg(rMax, zBinEdges, phiBinDeflectionCoverage, phi, maxPhiBins, impactMax)
                                SpacePointGridConfigArg settings. phi is specified as a tuple of (min,max).
        Defaults specified in Core/include/Acts/Seeding/SpacePointGrid.hpp
    seedingAlgorithmConfigArg : SeedingAlgorithmConfigArg(allowSeparateRMax, zBinNeighborsTop, zBinNeighborsBottom, numPhiNeighbors)
                                Defaults specified in Examples/Algorithms/TrackFinding/include/ActsExamples/TrackFinding/SeedingAlgorithm.hpp
    truthEstimatedSeedingAlgorithmConfigArg : TruthEstimatedSeedingAlgorithmConfigArg(deltaR)
        Currently only deltaR=(min,max) range specified here.
    particleHypothesis : Optional[acts.ParticleHypothesis]
        The hypothesis used for track finding. Defaults to pion.
    inputParticles : str, "particles"
        input particles name in the WhiteBoard
    outputDirRoot : Path|str, path, None
        the output folder for the Root output, None triggers no output
    logLevel : acts.logging.Level, None
        logging level to override setting given in `s`
    rnd : RandomNumbers, None
        random number generator. Only used by SeedingAlgorithm.TruthSmeared.
    """

    logLevel = acts.examples.defaultLogging(s, logLevel)()
    logger = acts.logging.getLogger("addSeeding")
    logger.setLevel(logLevel)

    if truthSeedRanges is not None:
        selectedParticles = "truth_seeds_selected"
        addSeedingTruthSelection(
            s,
            inputParticles,
            selectedParticles,
            truthSeedRanges,
            logLevel,
        )
    else:
        selectedParticles = inputParticles

    # Create starting parameters from either particle smearing or combined seed
    # finding and track parameters estimation
    if seedingAlgorithm == SeedingAlgorithm.TruthSmeared:
        logger.info("Using smeared truth particles for seeding")
        addTruthSmearedSeeding(
            s,
            rnd,
            selectedParticles,
            particleSmearingSigmas,
            initialSigmas,
            initialVarInflation,
            particleHypothesis,
            logLevel,
        )
    else:
        spacePoints = addSpacePointsMaking(
            s, trackingGeometry, geoSelectionConfigFile, logLevel
        )
        # Run either: truth track finding or seeding
        if seedingAlgorithm == SeedingAlgorithm.TruthEstimated:
            logger.info("Using truth track finding from space points for seeding")
            seeds = addTruthEstimatedSeeding(
                s,
                spacePoints,
                selectedParticles,
                truthEstimatedSeedingAlgorithmConfigArg,
                logLevel,
            )
        elif seedingAlgorithm == SeedingAlgorithm.Default:
            logger.info("Using default seeding")
            seeds = addStandardSeeding(
                s,
                spacePoints,
                seedingAlgorithmConfigArg,
                seedFinderConfigArg,
                seedFinderOptionsArg,
                seedFilterConfigArg,
                spacePointGridConfigArg,
                logLevel,
            )
        elif seedingAlgorithm == SeedingAlgorithm.Orthogonal:
            logger.info("Using orthogonal seeding")
            seeds = addOrthogonalSeeding(
                s,
                spacePoints,
                seedFinderConfigArg,
                seedFinderOptionsArg,
                seedFilterConfigArg,
                logLevel,
            )
        elif seedingAlgorithm == SeedingAlgorithm.HoughTransform:
            logger.info("Using Hough Transform seeding")
            houghTransformConfig.inputSpacePoints = [spacePoints]
            houghTransformConfig.inputMeasurements = "measurements"
            houghTransformConfig.inputSourceLinks = "sourcelinks"
            houghTransformConfig.outputProtoTracks = "prototracks"
            houghTransformConfig.outputSeeds = "seeds"
            houghTransformConfig.trackingGeometry = trackingGeometry
            seeds = addHoughTransformSeeding(s, houghTransformConfig, logLevel)
        elif seedingAlgorithm == SeedingAlgorithm.FTF:
            logger.info("Using FTF seeding")
            # output of algs changed, only one output now
            seeds = addFTFSeeding(
                s,
                spacePoints,
                seedFinderConfigArg,
                seedFinderOptionsArg,
                seedFilterConfigArg,
                trackingGeometry,
                logLevel,
                layerMappingConfigFile,
                geoSelectionConfigFile,
                fastrack_inputConfigFile,
            )
        else:
            logger.fatal("unknown seedingAlgorithm %s", seedingAlgorithm)

        parEstimateAlg = acts.examples.TrackParamsEstimationAlgorithm(
            level=logLevel,
            inputSeeds=seeds,
            outputTrackParameters="estimatedparameters",
            trackingGeometry=trackingGeometry,
            magneticField=field,
            **acts.examples.defaultKWArgs(
                initialSigmas=initialSigmas,
                initialVarInflation=initialVarInflation,
                particleHypothesis=particleHypothesis,
            ),
        )
        s.addAlgorithm(parEstimateAlg)

        prototracks = "seed-prototracks"
        s.addAlgorithm(
            acts.examples.SeedsToPrototracks(
                level=logLevel,
                inputSeeds=seeds,
                outputProtoTracks=prototracks,
            )
        )

        if outputDirRoot is not None:
            addSeedPerformanceWriters(
                s,
                outputDirRoot,
                seeds,
                prototracks,
                selectedParticles,
                inputParticles,
                parEstimateAlg.config.outputTrackParameters,
                logLevel,
            )

    return s


def addSeedingTruthSelection(
    s: acts.examples.Sequencer,
    inputParticles: str,
    outputParticles: str,
    truthSeedRanges: TruthSeedRanges,
    logLevel: acts.logging.Level = None,
):
    """adds truth particles filtering before filtering
    For parameters description see addSeeding
    """
    selAlg = acts.examples.TruthSeedSelector(
        **acts.examples.defaultKWArgs(
            ptMin=truthSeedRanges.pt[0],
            ptMax=truthSeedRanges.pt[1],
            etaMin=truthSeedRanges.eta[0],
            etaMax=truthSeedRanges.eta[1],
            nHitsMin=truthSeedRanges.nHits[0],
            nHitsMax=truthSeedRanges.nHits[1],
            rhoMin=truthSeedRanges.rho[0],
            rhoMax=truthSeedRanges.rho[1],
            zMin=truthSeedRanges.z[0],
            zMax=truthSeedRanges.z[1],
            phiMin=truthSeedRanges.phi[0],
            phiMax=truthSeedRanges.phi[1],
            absEtaMin=truthSeedRanges.absEta[0],
            absEtaMax=truthSeedRanges.absEta[1],
        ),
        level=logLevel,
        inputParticles=inputParticles,
        inputMeasurementParticlesMap="measurement_particles_map",
        outputParticles=outputParticles,
    )
    s.addAlgorithm(selAlg)


def addTruthSmearedSeeding(
    sequence: acts.examples.Sequencer,
    rnd: Optional[acts.examples.RandomNumbers],
    selectedParticles: str,
    particleSmearingSigmas: ParticleSmearingSigmas,
    initialSigmas: Optional[List[float]],
    initialVarInflation: List[float],
    particleHypothesis: Optional[acts.ParticleHypothesis],
    logLevel: acts.logging.Level = None,
):
    """adds algorithm that would mimic detector response uncertainties for truth seeding
    For parameters description see addSeeding
    """

    rnd = rnd or acts.examples.RandomNumbers(seed=42)
    # Run particle smearing
    ptclSmear = acts.examples.ParticleSmearing(
        level=logLevel,
        inputParticles=selectedParticles,
        outputTrackParameters="estimatedparameters",
        randomNumbers=rnd,
        # gaussian sigmas to smear particle parameters
        **acts.examples.defaultKWArgs(
            sigmaD0=particleSmearingSigmas.d0,
            sigmaD0PtA=particleSmearingSigmas.d0PtA,
            sigmaD0PtB=particleSmearingSigmas.d0PtB,
            sigmaZ0=particleSmearingSigmas.z0,
            sigmaZ0PtA=particleSmearingSigmas.z0PtA,
            sigmaZ0PtB=particleSmearingSigmas.z0PtB,
            sigmaT0=particleSmearingSigmas.t0,
            sigmaPhi=particleSmearingSigmas.phi,
            sigmaTheta=particleSmearingSigmas.theta,
            sigmaPRel=particleSmearingSigmas.pRel,
            initialSigmas=initialSigmas,
            initialVarInflation=initialVarInflation,
            particleHypothesis=particleHypothesis,
        ),
    )
    sequence.addAlgorithm(ptclSmear)

    truthTrkFndAlg = acts.examples.TruthTrackFinder(
        level=logLevel,
        inputParticles=selectedParticles,
        inputMeasurementParticlesMap="measurement_particles_map",
        outputProtoTracks="truth_particle_tracks",
    )
    sequence.addAlgorithm(truthTrkFndAlg)


def addTruthEstimatedSeeding(
    sequence: acts.examples.Sequencer,
    spacePoints: str,
    inputParticles: str,
    TruthEstimatedSeedingAlgorithmConfigArg: TruthEstimatedSeedingAlgorithmConfigArg,
    logLevel: acts.logging.Level = None,
):
    """adds truth seeding
    For parameters description see addSeeding
    """
    logLevel = acts.examples.defaultLogging(sequence, logLevel)()

    truthSeeding = acts.examples.TruthSeedingAlgorithm(
        level=logLevel,
        inputParticles=inputParticles,
        inputMeasurementParticlesMap="measurement_particles_map",
        inputSpacePoints=[spacePoints],
        outputParticles="truth_seeded_particles",
        outputProtoTracks="truth_particle_tracks",
        outputSeeds="seeds",
        **acts.examples.defaultKWArgs(
            deltaRMin=TruthEstimatedSeedingAlgorithmConfigArg.deltaR[0],
            deltaRMax=TruthEstimatedSeedingAlgorithmConfigArg.deltaR[1],
        ),
    )
    sequence.addAlgorithm(truthSeeding)

    return truthSeeding.config.outputSeeds


def addSpacePointsMaking(
    sequence: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    geoSelectionConfigFile: Union[Path, str],
    logLevel: acts.logging.Level = None,
):
    """adds space points making
    For parameters description see addSeeding
    """
    logLevel = acts.examples.defaultLogging(sequence, logLevel)()
    spAlg = acts.examples.SpacePointMaker(
        level=logLevel,
        inputSourceLinks="sourcelinks",
        inputMeasurements="measurements",
        outputSpacePoints="spacepoints",
        trackingGeometry=trackingGeometry,
        geometrySelection=acts.examples.readJsonGeometryList(
            str(geoSelectionConfigFile)
        ),
    )
    sequence.addAlgorithm(spAlg)
    return spAlg.config.outputSpacePoints


def addStandardSeeding(
    sequence: acts.examples.Sequencer,
    spacePoints: str,
    seedingAlgorithmConfigArg: SeedingAlgorithmConfigArg,
    seedFinderConfigArg: SeedFinderConfigArg,
    seedFinderOptionsArg: SeedFinderOptionsArg,
    seedFilterConfigArg: SeedFilterConfigArg,
    spacePointGridConfigArg: SpacePointGridConfigArg,
    logLevel: acts.logging.Level = None,
):
    """adds standard seeding
    For parameters description see addSeeding
    """
    logLevel = acts.examples.defaultLogging(sequence, logLevel)()

    seedFinderConfig = acts.SeedFinderConfig(
        **acts.examples.defaultKWArgs(
            rMin=seedFinderConfigArg.r[0],
            rMax=seedFinderConfigArg.r[1],
            deltaRMin=seedFinderConfigArg.deltaR[0],
            deltaRMax=seedFinderConfigArg.deltaR[1],
            deltaRMinTopSP=(
                seedFinderConfigArg.deltaR[0]
                if seedFinderConfigArg.deltaRTopSP[0] is None
                else seedFinderConfigArg.deltaRTopSP[0]
            ),
            deltaRMaxTopSP=(
                seedFinderConfigArg.deltaR[1]
                if seedFinderConfigArg.deltaRTopSP[1] is None
                else seedFinderConfigArg.deltaRTopSP[1]
            ),
            deltaRMinBottomSP=(
                seedFinderConfigArg.deltaR[0]
                if seedFinderConfigArg.deltaRBottomSP[0] is None
                else seedFinderConfigArg.deltaRBottomSP[0]
            ),
            deltaRMaxBottomSP=(
                seedFinderConfigArg.deltaR[1]
                if seedFinderConfigArg.deltaRBottomSP[1] is None
                else seedFinderConfigArg.deltaRBottomSP[1]
            ),
            deltaRMiddleMinSPRange=seedFinderConfigArg.deltaRMiddleSPRange[0],
            deltaRMiddleMaxSPRange=seedFinderConfigArg.deltaRMiddleSPRange[1],
            collisionRegionMin=seedFinderConfigArg.collisionRegion[0],
            collisionRegionMax=seedFinderConfigArg.collisionRegion[1],
            zMin=seedFinderConfigArg.z[0],
            zMax=seedFinderConfigArg.z[1],
            zOutermostLayers=(
                seedFinderConfigArg.zOutermostLayers[0]
                if seedFinderConfigArg.zOutermostLayers[0] is not None
                else seedFinderConfigArg.z[0],
                seedFinderConfigArg.zOutermostLayers[1]
                if seedFinderConfigArg.zOutermostLayers[1] is not None
                else seedFinderConfigArg.z[1],
            ),
            maxSeedsPerSpM=seedFinderConfigArg.maxSeedsPerSpM,
            cotThetaMax=seedFinderConfigArg.cotThetaMax,
            sigmaScattering=seedFinderConfigArg.sigmaScattering,
            radLengthPerSeed=seedFinderConfigArg.radLengthPerSeed,
            minPt=seedFinderConfigArg.minPt,
            impactMax=seedFinderConfigArg.impactMax,
            interactionPointCut=seedFinderConfigArg.interactionPointCut,
            deltaZMax=seedFinderConfigArg.deltaZMax,
            maxPtScattering=seedFinderConfigArg.maxPtScattering,
            zBinEdges=seedFinderConfigArg.zBinEdges,
            zBinsCustomLooping=seedFinderConfigArg.zBinsCustomLooping,
            skipZMiddleBinSearch=seedFinderConfigArg.skipZMiddleBinSearch,
            rRangeMiddleSP=seedFinderConfigArg.rRangeMiddleSP,
            useVariableMiddleSPRange=seedFinderConfigArg.useVariableMiddleSPRange,
            binSizeR=seedFinderConfigArg.binSizeR,
            seedConfirmation=seedFinderConfigArg.seedConfirmation,
            centralSeedConfirmationRange=seedFinderConfigArg.centralSeedConfirmationRange,
            forwardSeedConfirmationRange=seedFinderConfigArg.forwardSeedConfirmationRange,
        ),
    )
    seedFinderOptions = acts.SeedFinderOptions(
        **acts.examples.defaultKWArgs(
            beamPos=acts.Vector2(0.0, 0.0)
            if seedFinderOptionsArg.beamPos == (None, None)
            else acts.Vector2(
                seedFinderOptionsArg.beamPos[0], seedFinderOptionsArg.beamPos[1]
            ),
            bFieldInZ=seedFinderOptionsArg.bFieldInZ,
        )
    )
    seedFilterConfig = acts.SeedFilterConfig(
        **acts.examples.defaultKWArgs(
            maxSeedsPerSpM=seedFinderConfig.maxSeedsPerSpM,
            deltaRMin=(
                seedFinderConfig.deltaRMin
                if seedFilterConfigArg.deltaRMin is None
                else seedFilterConfigArg.deltaRMin
            ),
            impactWeightFactor=seedFilterConfigArg.impactWeightFactor,
            zOriginWeightFactor=seedFilterConfigArg.zOriginWeightFactor,
            compatSeedWeight=seedFilterConfigArg.compatSeedWeight,
            compatSeedLimit=seedFilterConfigArg.compatSeedLimit,
            numSeedIncrement=seedFilterConfigArg.numSeedIncrement,
            seedWeightIncrement=seedFilterConfigArg.seedWeightIncrement,
            seedConfirmation=seedFilterConfigArg.seedConfirmation,
            centralSeedConfirmationRange=seedFinderConfig.centralSeedConfirmationRange,
            forwardSeedConfirmationRange=seedFinderConfig.forwardSeedConfirmationRange,
            maxSeedsPerSpMConf=seedFilterConfigArg.maxSeedsPerSpMConf,
            maxQualitySeedsPerSpMConf=seedFilterConfigArg.maxQualitySeedsPerSpMConf,
            useDeltaRorTopRadius=seedFilterConfigArg.useDeltaRorTopRadius,
        )
    )

    gridConfig = acts.SpacePointGridConfig(
        **acts.examples.defaultKWArgs(
            minPt=seedFinderConfig.minPt,
            rMax=(
                seedFinderConfig.rMax
                if spacePointGridConfigArg.rMax is None
                else spacePointGridConfigArg.rMax
            ),
            zMax=seedFinderConfig.zMax,
            zMin=seedFinderConfig.zMin,
            deltaRMax=(
                seedFinderConfig.deltaRMax
                if spacePointGridConfigArg.deltaRMax is None
                else spacePointGridConfigArg.deltaRMax
            ),
            cotThetaMax=seedFinderConfig.cotThetaMax,
            phiMin=spacePointGridConfigArg.phi[0],
            phiMax=spacePointGridConfigArg.phi[1],
            maxPhiBins=spacePointGridConfigArg.maxPhiBins,
            impactMax=spacePointGridConfigArg.impactMax,
            zBinEdges=spacePointGridConfigArg.zBinEdges,
            phiBinDeflectionCoverage=spacePointGridConfigArg.phiBinDeflectionCoverage,
        )
    )

    gridOptions = acts.SpacePointGridOptions(
        **acts.examples.defaultKWArgs(
            bFieldInZ=seedFinderOptions.bFieldInZ,
        )
    )

    seedingAlg = acts.examples.SeedingAlgorithm(
        level=logLevel,
        inputSpacePoints=[spacePoints],
        outputSeeds="seeds",
        **acts.examples.defaultKWArgs(
            allowSeparateRMax=seedingAlgorithmConfigArg.allowSeparateRMax,
            zBinNeighborsTop=seedingAlgorithmConfigArg.zBinNeighborsTop,
            zBinNeighborsBottom=seedingAlgorithmConfigArg.zBinNeighborsBottom,
            numPhiNeighbors=seedingAlgorithmConfigArg.numPhiNeighbors,
        ),
        gridConfig=gridConfig,
        gridOptions=gridOptions,
        seedFilterConfig=seedFilterConfig,
        seedFinderConfig=seedFinderConfig,
        seedFinderOptions=seedFinderOptions,
    )
    sequence.addAlgorithm(seedingAlg)

    return seedingAlg.config.outputSeeds


def addOrthogonalSeeding(
    sequence: acts.examples.Sequencer,
    spacePoints: str,
    seedFinderConfigArg: SeedFinderConfigArg,
    seedFinderOptionsArg: SeedFinderOptionsArg,
    seedFilterConfigArg: SeedFilterConfigArg,
    logLevel: acts.logging.Level = None,
):
    """adds orthogonal seeding algorithm
    For parameters description see addSeeding
    """
    logLevel = acts.examples.defaultLogging(sequence, logLevel)()
    seedFinderConfig = acts.SeedFinderOrthogonalConfig(
        **acts.examples.defaultKWArgs(
            rMin=seedFinderConfigArg.r[0],
            rMax=seedFinderConfigArg.r[1],
            deltaRMinTopSP=(
                seedFinderConfigArg.deltaR[0]
                if seedFinderConfigArg.deltaRTopSP[0] is None
                else seedFinderConfigArg.deltaRTopSP[0]
            ),
            deltaRMaxTopSP=(
                seedFinderConfigArg.deltaR[1]
                if seedFinderConfigArg.deltaRTopSP[1] is None
                else seedFinderConfigArg.deltaRTopSP[1]
            ),
            deltaRMinBottomSP=(
                seedFinderConfigArg.deltaR[0]
                if seedFinderConfigArg.deltaRBottomSP[0] is None
                else seedFinderConfigArg.deltaRBottomSP[0]
            ),
            deltaRMaxBottomSP=(
                seedFinderConfigArg.deltaR[1]
                if seedFinderConfigArg.deltaRBottomSP[1] is None
                else seedFinderConfigArg.deltaRBottomSP[1]
            ),
            collisionRegionMin=seedFinderConfigArg.collisionRegion[0],
            collisionRegionMax=seedFinderConfigArg.collisionRegion[1],
            zMin=seedFinderConfigArg.z[0],
            zMax=seedFinderConfigArg.z[1],
            zOutermostLayers=(
                seedFinderConfigArg.zOutermostLayers[0]
                if seedFinderConfigArg.zOutermostLayers[0] is not None
                else seedFinderConfigArg.z[0],
                seedFinderConfigArg.zOutermostLayers[1]
                if seedFinderConfigArg.zOutermostLayers[1] is not None
                else seedFinderConfigArg.z[1],
            ),
            maxSeedsPerSpM=seedFinderConfigArg.maxSeedsPerSpM,
            cotThetaMax=seedFinderConfigArg.cotThetaMax,
            sigmaScattering=seedFinderConfigArg.sigmaScattering,
            radLengthPerSeed=seedFinderConfigArg.radLengthPerSeed,
            minPt=seedFinderConfigArg.minPt,
            impactMax=seedFinderConfigArg.impactMax,
            deltaPhiMax=seedFinderConfigArg.deltaPhiMax,
            interactionPointCut=seedFinderConfigArg.interactionPointCut,
            deltaZMax=seedFinderConfigArg.deltaZMax,
            maxPtScattering=seedFinderConfigArg.maxPtScattering,
            rRangeMiddleSP=seedFinderConfigArg.rRangeMiddleSP,
            useVariableMiddleSPRange=seedFinderConfigArg.useVariableMiddleSPRange,
            seedConfirmation=seedFinderConfigArg.seedConfirmation,
            centralSeedConfirmationRange=seedFinderConfigArg.centralSeedConfirmationRange,
            forwardSeedConfirmationRange=seedFinderConfigArg.forwardSeedConfirmationRange,
        ),
    )
    seedFinderOptions = acts.SeedFinderOptions(
        **acts.examples.defaultKWArgs(
            beamPos=acts.Vector2(0.0, 0.0)
            if seedFinderOptionsArg.beamPos == (None, None)
            else acts.Vector2(
                seedFinderOptionsArg.beamPos[0], seedFinderOptionsArg.beamPos[1]
            ),
            bFieldInZ=seedFinderOptionsArg.bFieldInZ,
        )
    )
    seedFilterConfig = acts.SeedFilterConfig(
        **acts.examples.defaultKWArgs(
            maxSeedsPerSpM=seedFinderConfig.maxSeedsPerSpM,
            deltaRMin=(
                seedFinderConfigArg.deltaR[0]
                if seedFilterConfigArg.deltaRMin is None
                else seedFilterConfigArg.deltaRMin
            ),
            impactWeightFactor=seedFilterConfigArg.impactWeightFactor,
            zOriginWeightFactor=seedFilterConfigArg.zOriginWeightFactor,
            compatSeedWeight=seedFilterConfigArg.compatSeedWeight,
            compatSeedLimit=seedFilterConfigArg.compatSeedLimit,
            numSeedIncrement=seedFilterConfigArg.numSeedIncrement,
            seedWeightIncrement=seedFilterConfigArg.seedWeightIncrement,
            seedConfirmation=seedFilterConfigArg.seedConfirmation,
            maxSeedsPerSpMConf=seedFilterConfigArg.maxSeedsPerSpMConf,
            maxQualitySeedsPerSpMConf=seedFilterConfigArg.maxQualitySeedsPerSpMConf,
            useDeltaRorTopRadius=seedFilterConfigArg.useDeltaRorTopRadius,
        )
    )
    seedingAlg = acts.examples.SeedingOrthogonalAlgorithm(
        level=logLevel,
        inputSpacePoints=[spacePoints],
        outputSeeds="seeds",
        seedFilterConfig=seedFilterConfig,
        seedFinderConfig=seedFinderConfig,
        seedFinderOptions=seedFinderOptions,
    )
    sequence.addAlgorithm(seedingAlg)

    return seedingAlg.config.outputSeeds


def addHoughTransformSeeding(
    sequence: acts.examples.Sequencer,
    config: acts.examples.HoughTransformSeeder.Config,
    logLevel: acts.logging.Level = None,
):
    """
    Configures HoughTransform (HT) for seeding, instead of extra proxy config objects it takes
    directly the HT example algorithm config.
    """
    logLevel = acts.examples.defaultLogging(sequence, logLevel)()
    ht = acts.examples.HoughTransformSeeder(config=config, level=logLevel)
    sequence.addAlgorithm(ht)
    # potentially HT can be extended to also produce seeds, but it is not implemented yet
    # configuration option (outputSeeds) exists
    return ht.config.outputSeeds


def addFTFSeeding(
    sequence: acts.examples.Sequencer,
    spacePoints: str,
    seedFinderConfigArg: SeedFinderConfigArg,
    seedFinderOptionsArg: SeedFinderOptionsArg,
    seedFilterConfigArg: SeedFilterConfigArg,
    trackingGeometry: acts.TrackingGeometry,
    logLevel: acts.logging.Level = None,
    layerMappingConfigFile: Union[Path, str] = None,
    geoSelectionConfigFile: Union[Path, str] = None,
    fastrack_inputConfigFile: Union[Path, str] = None,
):
    """FTF seeding"""

    logLevel = acts.examples.defaultLogging(sequence, logLevel)()
    layerMappingFile = str(layerMappingConfigFile)  # turn path into string
    fastrack_inputFile = str(fastrack_inputConfigFile)
    seedFinderConfig = acts.SeedFinderFTFConfig(
        **acts.examples.defaultKWArgs(
            sigmaScattering=seedFinderConfigArg.sigmaScattering,
            maxSeedsPerSpM=seedFinderConfigArg.maxSeedsPerSpM,
            minPt=seedFinderConfigArg.minPt,
            fastrack_input_file=fastrack_inputFile,
            m_useClusterWidth=False,
        ),
    )
    seedFinderOptions = acts.SeedFinderOptions(
        **acts.examples.defaultKWArgs(
            beamPos=acts.Vector2(0.0, 0.0)
            if seedFinderOptionsArg.beamPos == (None, None)
            else acts.Vector2(
                seedFinderOptionsArg.beamPos[0], seedFinderOptionsArg.beamPos[1]
            ),
            bFieldInZ=seedFinderOptionsArg.bFieldInZ,
        )
    )
    seedFilterConfig = acts.SeedFilterConfig(
        **acts.examples.defaultKWArgs(
            maxSeedsPerSpM=seedFinderConfig.maxSeedsPerSpM,
            deltaRMin=(
                seedFinderConfigArg.deltaR[0]
                if seedFilterConfigArg.deltaRMin is None
                else seedFilterConfigArg.deltaRMin
            ),
            impactWeightFactor=seedFilterConfigArg.impactWeightFactor,
            zOriginWeightFactor=seedFilterConfigArg.zOriginWeightFactor,
            compatSeedWeight=seedFilterConfigArg.compatSeedWeight,
            compatSeedLimit=seedFilterConfigArg.compatSeedLimit,
            numSeedIncrement=seedFilterConfigArg.numSeedIncrement,
            seedWeightIncrement=seedFilterConfigArg.seedWeightIncrement,
            seedConfirmation=seedFilterConfigArg.seedConfirmation,
            # curvatureSortingInFilter=seedFilterConfigArg.curvatureSortingInFilter,
            maxSeedsPerSpMConf=seedFilterConfigArg.maxSeedsPerSpMConf,
            maxQualitySeedsPerSpMConf=seedFilterConfigArg.maxQualitySeedsPerSpMConf,
            useDeltaRorTopRadius=seedFilterConfigArg.useDeltaRorTopRadius,
        )
    )

    seedingAlg = acts.examples.SeedingFTFAlgorithm(
        level=logLevel,
        inputSpacePoints=[spacePoints],
        outputSeeds="seeds",
        seedFilterConfig=seedFilterConfig,
        seedFinderConfig=seedFinderConfig,
        seedFinderOptions=seedFinderOptions,
        layerMappingFile=layerMappingFile,
        geometrySelection=acts.examples.readJsonGeometryList(
            str(geoSelectionConfigFile)
        ),
        inputSourceLinks="sourcelinks",
        trackingGeometry=trackingGeometry,
        fill_module_csv=False,
    )

    sequence.addAlgorithm(seedingAlg)
    return seedingAlg.config.outputSeeds


def addSeedPerformanceWriters(
    sequence: acts.examples.Sequencer,
    outputDirRoot: Union[Path, str],
    seeds: str,
    prototracks: str,
    selectedParticles: str,
    inputParticles: str,
    outputTrackParameters: str,
    logLevel: acts.logging.Level = None,
):
    """Writes seeding related performance output"""
    customLogLevel = acts.examples.defaultLogging(sequence, logLevel)
    outputDirRoot = Path(outputDirRoot)
    if not outputDirRoot.exists():
        outputDirRoot.mkdir()

    sequence.addWriter(
        acts.examples.SeedingPerformanceWriter(
            level=customLogLevel(minLevel=acts.logging.DEBUG),
            inputSeeds=seeds,
            inputParticles=selectedParticles,
            inputMeasurementParticlesMap="measurement_particles_map",
            filePath=str(outputDirRoot / "performance_seeding.root"),
        )
    )

    sequence.addWriter(
        acts.examples.RootTrackParameterWriter(
            level=customLogLevel(),
            inputTrackParameters=outputTrackParameters,
            inputProtoTracks=prototracks,
            inputParticles=inputParticles,
            inputSimHits="simhits",
            inputMeasurementParticlesMap="measurement_particles_map",
            inputMeasurementSimHitsMap="measurement_simhits_map",
            filePath=str(outputDirRoot / "estimatedparams.root"),
            treeName="estimatedparams",
        )
    )


def addKalmanTracks(
    s: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    directNavigation: bool = False,
    reverseFilteringMomThreshold: float = 0 * u.GeV,
    inputProtoTracks: str = "truth_particle_tracks",
    multipleScattering: bool = True,
    energyLoss: bool = True,
    clusters: str = None,
    calibrator: acts.examples.MeasurementCalibrator = acts.examples.makePassThroughCalibrator(),
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    if directNavigation:
        srfSortAlg = acts.examples.SurfaceSortingAlgorithm(
            level=customLogLevel(),
            inputProtoTracks=inputProtoTracks,
            inputSimHits="simhits",
            inputMeasurementSimHitsMap="measurement_simhits_map",
            outputProtoTracks="sorted_truth_particle_tracks",
        )
        s.addAlgorithm(srfSortAlg)
        inputProtoTracks = srfSortAlg.config.outputProtoTracks

    kalmanOptions = {
        "multipleScattering": multipleScattering,
        "energyLoss": energyLoss,
        "reverseFilteringMomThreshold": reverseFilteringMomThreshold,
        "freeToBoundCorrection": acts.examples.FreeToBoundCorrection(False),
        "level": customLogLevel(),
    }

    fitAlg = acts.examples.TrackFittingAlgorithm(
        level=customLogLevel(),
        inputMeasurements="measurements",
        inputSourceLinks="sourcelinks",
        inputProtoTracks=inputProtoTracks,
        inputInitialTrackParameters="estimatedparameters",
        inputClusters=clusters if clusters is not None else "",
        outputTracks="kfTracks",
        pickTrack=-1,
        fit=acts.examples.makeKalmanFitterFunction(
            trackingGeometry, field, **kalmanOptions
        ),
        calibrator=calibrator,
    )
    s.addAlgorithm(fitAlg)
    s.addWhiteboardAlias("tracks", fitAlg.config.outputTracks)

    return s


def addTruthTrackingGsf(
    s: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    inputProtoTracks: str = "truth_particle_tracks",
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    gsfOptions = {
        "betheHeitlerApprox": acts.examples.AtlasBetheHeitlerApprox.makeDefault(),
        "maxComponents": 12,
        "componentMergeMethod": acts.examples.ComponentMergeMethod.maxWeight,
        "mixtureReductionAlgorithm": acts.examples.MixtureReductionAlgorithm.KLDistance,
        "weightCutoff": 1.0e-4,
        "momentumCutoff": 100 * u.MeV,
        "level": customLogLevel(),
    }

    gsfAlg = acts.examples.TrackFittingAlgorithm(
        level=customLogLevel(),
        inputMeasurements="measurements",
        inputSourceLinks="sourcelinks",
        inputProtoTracks=inputProtoTracks,
        inputInitialTrackParameters="estimatedparameters",
        outputTracks="gsf_tracks",
        pickTrack=-1,
        fit=acts.examples.makeGsfFitterFunction(trackingGeometry, field, **gsfOptions),
        calibrator=acts.examples.makePassThroughCalibrator(),
    )
    s.addAlgorithm(gsfAlg)
    s.addWhiteboardAlias("tracks", gsfAlg.config.outputTracks)

    return s


@acts.examples.NamedTypeArgs(
    trackSelectorConfig=TrackSelectorConfig,
    ckfConfig=CkfConfig,
)
def addCKFTracks(
    s: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    trackSelectorConfig: Optional[TrackSelectorConfig] = None,
    ckfConfig: CkfConfig = CkfConfig(),
    outputDirCsv: Optional[Union[Path, str]] = None,
    outputDirRoot: Optional[Union[Path, str]] = None,
    writeTrajectories: bool = True,
    logLevel: Optional[acts.logging.Level] = None,
    writeCovMat=False,
) -> None:
    """This function steers the seeding

    Parameters
    ----------
    s: Sequencer
        the sequencer module to which we add the Seeding steps (returned from addSeeding)
    trackingGeometry : tracking geometry
    field : magnetic field
    outputDirCsv : Path|str, path, None
        the output folder for the Csv output, None triggers no output
    outputDirRoot : Path|str, path, None
        the output folder for the Root output, None triggers no output
    trackSelectorConfig : TrackSelectorConfig(loc0, loc1, time, eta, absEta, pt, phi, minMeasurements)
        TrackSelector configuration. Each range is specified as a tuple of (min,max).
        Defaults of no cuts specified in Examples/Algorithms/TruthTracking/ActsExamples/TruthTracking/TrackSelector.hpp
    writeTrajectories : bool, True
        write trackstates_ckf.root and tracksummary_ckf.root ntuples? These can be quite large.
    """

    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    # Setup the track finding algorithm with CKF
    # It takes all the source links created from truth hit smearing, seeds from
    # truth particle smearing and source link selection config
    trackFinder = acts.examples.TrackFindingAlgorithm(
        level=customLogLevel(),
        measurementSelectorCfg=acts.MeasurementSelector.Config(
            [
                (
                    acts.GeometryIdentifier(),
                    (
                        [],
                        [ckfConfig.chi2CutOff],
                        [ckfConfig.numMeasurementsCutOff],
                    ),
                )
            ]
        ),
        trackSelectorCfg=acts.TrackSelector.Config(
            **acts.examples.defaultKWArgs(
                loc0Min=trackSelectorConfig.loc0[0],
                loc0Max=trackSelectorConfig.loc0[1],
                loc1Min=trackSelectorConfig.loc1[0],
                loc1Max=trackSelectorConfig.loc1[1],
                timeMin=trackSelectorConfig.time[0],
                timeMax=trackSelectorConfig.time[1],
                phiMin=trackSelectorConfig.phi[0],
                phiMax=trackSelectorConfig.phi[1],
                etaMin=trackSelectorConfig.eta[0],
                etaMax=trackSelectorConfig.eta[1],
                absEtaMin=trackSelectorConfig.absEta[0],
                absEtaMax=trackSelectorConfig.absEta[1],
                ptMin=trackSelectorConfig.pt[0],
                ptMax=trackSelectorConfig.pt[1],
                minMeasurements=trackSelectorConfig.nMeasurementsMin,
            )
        )
        if trackSelectorConfig is not None
        else None,
        inputMeasurements="measurements",
        inputSourceLinks="sourcelinks",
        inputInitialTrackParameters="estimatedparameters",
        outputTracks="ckfTracks",
        findTracks=acts.examples.TrackFindingAlgorithm.makeTrackFinderFunction(
            trackingGeometry, field, customLogLevel()
        ),
        **acts.examples.defaultKWArgs(
            maxSteps=ckfConfig.maxSteps,
        ),
    )
    s.addAlgorithm(trackFinder)
    s.addWhiteboardAlias("tracks", trackFinder.config.outputTracks)

    addTrackWriters(
        s,
        name="ckf",
        tracks=trackFinder.config.outputTracks,
        outputDirCsv=outputDirCsv,
        outputDirRoot=outputDirRoot,
        writeStates=writeTrajectories,
        writeSummary=writeTrajectories,
        writeCKFperformance=True,
        writeFinderPerformance=False,
        writeFitterPerformance=False,
        logLevel=logLevel,
        writeCovMat=writeCovMat,
    )

    return s


def addGx2fTracks(
    s: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    field: acts.MagneticFieldProvider,
    inputProtoTracks: str = "truth_particle_tracks",
    multipleScattering: bool = False,
    energyLoss: bool = False,
    nUpdateMax: int = 5,
    zeroField: bool = False,
    relChi2changeCutOff: float = 1e-7,
    clusters: str = None,
    calibrator: acts.examples.MeasurementCalibrator = acts.examples.makePassThroughCalibrator(),
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    gx2fOptions = {
        "multipleScattering": multipleScattering,
        "energyLoss": energyLoss,
        "freeToBoundCorrection": acts.examples.FreeToBoundCorrection(False),
        "nUpdateMax": nUpdateMax,
        "zeroField": zeroField,
        "relChi2changeCutOff": relChi2changeCutOff,
        "level": customLogLevel(),
    }

    fitAlg = acts.examples.TrackFittingAlgorithm(
        level=customLogLevel(),
        inputMeasurements="measurements",
        inputSourceLinks="sourcelinks",
        inputProtoTracks=inputProtoTracks,
        inputInitialTrackParameters="estimatedparameters",
        inputClusters=clusters if clusters is not None else "",
        outputTracks="gx2fTracks",
        pickTrack=-1,
        fit=acts.examples.makeGlobalChiSquareFitterFunction(
            trackingGeometry, field, **gx2fOptions
        ),
        calibrator=calibrator,
    )
    s.addAlgorithm(fitAlg)
    s.addWhiteboardAlias("tracks", fitAlg.config.outputTracks)

    return s


def addTrackWriters(
    s: acts.examples.Sequencer,
    name: str,
    tracks: str = "tracks",
    outputDirCsv: Optional[Union[Path, str]] = None,
    outputDirRoot: Optional[Union[Path, str]] = None,
    writeStates: bool = True,
    writeSummary: bool = True,
    writeCKFperformance: bool = True,
    writeFinderPerformance: bool = True,
    writeFitterPerformance: bool = True,
    logLevel: Optional[acts.logging.Level] = None,
    writeCovMat=False,
):
    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    if outputDirRoot is not None:
        outputDirRoot = Path(outputDirRoot)
        if not outputDirRoot.exists():
            outputDirRoot.mkdir()

        if writeStates:
            # write track states from CKF
            trackStatesWriter = acts.examples.RootTrackStatesWriter(
                level=customLogLevel(),
                inputTracks=tracks,
                # @note The full particles collection is used here to avoid lots of warnings
                # since the unselected CKF track might have a majority particle not in the
                # filtered particle collection. This could be avoided when a separate track
                # selection algorithm is used.
                inputParticles="particles_selected",
                inputSimHits="simhits",
                inputMeasurementParticlesMap="measurement_particles_map",
                inputMeasurementSimHitsMap="measurement_simhits_map",
                filePath=str(outputDirRoot / f"trackstates_{name}.root"),
                treeName="trackstates",
            )
            s.addWriter(trackStatesWriter)

        if writeSummary:
            # write track summary from CKF
            trackSummaryWriter = acts.examples.RootTrackSummaryWriter(
                level=customLogLevel(),
                inputTracks=tracks,
                # @note The full particles collection is used here to avoid lots of warnings
                # since the unselected CKF track might have a majority particle not in the
                # filtered particle collection. This could be avoided when a separate track
                # selection algorithm is used.
                inputParticles="particles_selected",
                inputMeasurementParticlesMap="measurement_particles_map",
                filePath=str(outputDirRoot / f"tracksummary_{name}.root"),
                treeName="tracksummary",
                writeCovMat=writeCovMat,
            )
            s.addWriter(trackSummaryWriter)

        if writeCKFperformance:
            # Write CKF performance data
            ckfPerfWriter = acts.examples.CKFPerformanceWriter(
                level=customLogLevel(),
                inputParticles="truth_seeds_selected",
                inputTracks=tracks,
                inputMeasurementParticlesMap="measurement_particles_map",
                filePath=str(outputDirRoot / f"performance_{name}.root"),
            )
            s.addWriter(ckfPerfWriter)

        if writeFinderPerformance:
            s.addWriter(
                acts.examples.TrackFinderPerformanceWriter(
                    level=acts.logging.INFO,
                    inputProtoTracks="prototracks",
                    inputParticles="truth_seeds_selected",
                    inputMeasurementParticlesMap="measurement_particles_map",
                    filePath=str(
                        outputDirRoot / f"performance_track_finder_{name}.root"
                    ),
                )
            )

        if writeFitterPerformance:
            s.addWriter(
                acts.examples.TrackFitterPerformanceWriter(
                    level=acts.logging.INFO,
                    inputParticles="truth_seeds_selected",
                    inputTracks=tracks,
                    inputMeasurementParticlesMap="measurement_particles_map",
                    filePath=str(
                        outputDirRoot / f"performance_track_fitter_{name}.root"
                    ),
                )
            )

    if outputDirCsv is not None:
        outputDirCsv = Path(outputDirCsv)
        if not outputDirCsv.exists():
            outputDirCsv.mkdir()

        if writeSummary:
            csvWriter = acts.examples.CsvTrackWriter(
                level=customLogLevel(),
                inputTracks=tracks,
                inputMeasurementParticlesMap="measurement_particles_map",
                outputDir=str(outputDirCsv),
                fileName=str(f"tracks_{name}.csv"),
            )
            s.addWriter(csvWriter)


@acts.examples.NamedTypeArgs(
    trackSelectorConfig=TrackSelectorConfig,
)
def addTrackSelection(
    s: acts.examples.Sequencer,
    trackSelectorConfig: TrackSelectorConfig,
    inputTracks: str,
    outputTracks: str,
    logLevel: Optional[acts.logging.Level] = None,
) -> acts.examples.TrackSelectorAlgorithm:
    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    # single cut config for implicit single bin eta configuration
    selectorConfig = acts.TrackSelector.Config(
        **acts.examples.defaultKWArgs(
            loc0Min=trackSelectorConfig.loc0[0],
            loc0Max=trackSelectorConfig.loc0[1],
            loc1Min=trackSelectorConfig.loc1[0],
            loc1Max=trackSelectorConfig.loc1[1],
            timeMin=trackSelectorConfig.time[0],
            timeMax=trackSelectorConfig.time[1],
            phiMin=trackSelectorConfig.phi[0],
            phiMax=trackSelectorConfig.phi[1],
            etaMin=trackSelectorConfig.eta[0],
            etaMax=trackSelectorConfig.eta[1],
            absEtaMin=trackSelectorConfig.absEta[0],
            absEtaMax=trackSelectorConfig.absEta[1],
            ptMin=trackSelectorConfig.pt[0],
            ptMax=trackSelectorConfig.pt[1],
            minMeasurements=trackSelectorConfig.nMeasurementsMin,
        )
    )

    trackSelector = acts.examples.TrackSelectorAlgorithm(
        level=customLogLevel(),
        inputTracks=inputTracks,
        outputTracks=outputTracks,
        selectorConfig=selectorConfig,
    )

    s.addAlgorithm(trackSelector)

    return trackSelector


ExaTrkXBackend = Enum("ExaTrkXBackend", "Torch Onnx")


def addExaTrkX(
    s: acts.examples.Sequencer,
    trackingGeometry: acts.TrackingGeometry,
    geometrySelection: Union[Path, str],
    modelDir: Union[Path, str],
    outputDirRoot: Optional[Union[Path, str]] = None,
    backend: Optional[ExaTrkXBackend] = ExaTrkXBackend.Torch,
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    # Run the particle selection
    # The pre-selection will select truth particles satisfying provided criteria
    # from all particles read in by particle reader for further processing. It
    # has no impact on the truth hits themselves
    s.addAlgorithm(
        acts.examples.TruthSeedSelector(
            level=customLogLevel(),
            ptMin=500 * u.MeV,
            nHitsMin=9,
            inputParticles="particles_initial",
            inputMeasurementParticlesMap="measurement_particles_map",
            outputParticles="particles_seed_selected",
        )
    )

    # Create space points
    s.addAlgorithm(
        acts.examples.SpacePointMaker(
            level=customLogLevel(),
            inputSourceLinks="sourcelinks",
            inputMeasurements="measurements",
            outputSpacePoints="spacepoints",
            trackingGeometry=trackingGeometry,
            geometrySelection=acts.examples.readJsonGeometryList(
                str(geometrySelection)
            ),
        )
    )

    metricLearningConfig = {
        "level": customLogLevel(),
        "embeddingDim": 8,
        "rVal": 1.6,
        "knnVal": 100,
    }

    filterConfig = {
        "level": customLogLevel(),
        "cut": 0.01,
    }

    gnnConfig = {
        "level": customLogLevel(),
        "cut": 0.5,
    }

    if backend == ExaTrkXBackend.Torch:
        metricLearningConfig["modelPath"] = str(modelDir / "embed.pt")
        metricLearningConfig["numFeatures"] = 3
        filterConfig["modelPath"] = str(modelDir / "filter.pt")
        filterConfig["nChunks"] = 10
        filterConfig["numFeatures"] = 3
        gnnConfig["modelPath"] = str(modelDir / "gnn.pt")
        gnnConfig["undirected"] = True
        gnnConfig["numFeatures"] = 3

        graphConstructor = acts.examples.TorchMetricLearning(**metricLearningConfig)
        edgeClassifiers = [
            acts.examples.TorchEdgeClassifier(**filterConfig),
            acts.examples.TorchEdgeClassifier(**gnnConfig),
        ]
        trackBuilder = acts.examples.BoostTrackBuilding(customLogLevel())
    elif backend == ExaTrkXBackend.Onnx:
        metricLearningConfig["modelPath"] = str(modelDir / "embedding.onnx")
        metricLearningConfig["spacepointFeatures"] = 3
        filterConfig["modelPath"] = str(modelDir / "filtering.onnx")
        gnnConfig["modelPath"] = str(modelDir / "gnn.onnx")

        graphConstructor = acts.examples.OnnxMetricLearning(**metricLearningConfig)
        edgeClassifiers = [
            acts.examples.OnnxEdgeClassifier(**filterConfig),
            acts.examples.OnnxEdgeClassifier(**gnnConfig),
        ]
        trackBuilder = acts.examples.CugraphTrackBuilding(customLogLevel())

    s.addAlgorithm(
        acts.examples.TrackFindingAlgorithmExaTrkX(
            level=customLogLevel(),
            inputSpacePoints="spacepoints",
            outputProtoTracks="protoTracks",
            graphConstructor=graphConstructor,
            edgeClassifiers=edgeClassifiers,
            trackBuilder=trackBuilder,
        )
    )

    # Write truth track finding / seeding performance
    if outputDirRoot is not None:
        s.addWriter(
            acts.examples.TrackFinderPerformanceWriter(
                level=customLogLevel(),
                inputProtoTracks="protoTracks",
                inputParticles="particles_initial",  # the original selected particles after digitization
                inputMeasurementParticlesMap="measurement_particles_map",
                filePath=str(Path(outputDirRoot) / "performance_track_finding.root"),
            )
        )

    return s


@acts.examples.NamedTypeArgs(
    config=AmbiguityResolutionConfig,
)
def addAmbiguityResolution(
    s,
    config: AmbiguityResolutionConfig = AmbiguityResolutionConfig(),
    tracks: str = "tracks",
    outputDirCsv: Optional[Union[Path, str]] = None,
    outputDirRoot: Optional[Union[Path, str]] = None,
    writeTrajectories: bool = True,
    logLevel: Optional[acts.logging.Level] = None,
    writeCovMat=False,
) -> None:
    from acts.examples import GreedyAmbiguityResolutionAlgorithm

    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    alg = GreedyAmbiguityResolutionAlgorithm(
        level=customLogLevel(),
        inputTracks=tracks,
        outputTracks="ambiTracks",
        **acts.examples.defaultKWArgs(
            maximumSharedHits=config.maximumSharedHits,
            nMeasurementsMin=config.nMeasurementsMin,
            maximumIterations=config.maximumIterations,
        ),
    )
    s.addAlgorithm(alg)
    s.addWhiteboardAlias("tracks", alg.config.outputTracks)

    addTrackWriters(
        s,
        name="ambi",
        tracks=alg.config.outputTracks,
        outputDirCsv=outputDirCsv,
        outputDirRoot=outputDirRoot,
        writeStates=writeTrajectories,
        writeSummary=writeTrajectories,
        writeCKFperformance=True,
        writeFinderPerformance=False,
        writeFitterPerformance=False,
        logLevel=logLevel,
        writeCovMat=writeCovMat,
    )

    return s


@acts.examples.NamedTypeArgs(
    config=AmbiguityResolutionMLConfig,
)
def addAmbiguityResolutionML(
    s,
    config: AmbiguityResolutionMLConfig = AmbiguityResolutionMLConfig(),
    onnxModelFile: Optional[Union[Path, str]] = None,
    outputDirCsv: Optional[Union[Path, str]] = None,
    outputDirRoot: Optional[Union[Path, str]] = None,
    writeTrajectories: bool = True,
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    from acts.examples.onnx import AmbiguityResolutionMLAlgorithm
    from acts.examples import GreedyAmbiguityResolutionAlgorithm

    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    algML = AmbiguityResolutionMLAlgorithm(
        level=customLogLevel(),
        inputTracks="tracks",
        inputDuplicateNN=onnxModelFile,
        outputTracks="ambiTracksML",
        **acts.examples.defaultKWArgs(
            nMeasurementsMin=config.nMeasurementsMin,
        ),
    )

    algGreedy = GreedyAmbiguityResolutionAlgorithm(
        level=customLogLevel(),
        inputTracks=algML.config.outputTracks,
        outputTracks="ambiTracksMLGreedy",
        **acts.examples.defaultKWArgs(
            maximumSharedHits=config.maximumSharedHits,
            nMeasurementsMin=config.nMeasurementsMin,
            maximumIterations=config.maximumIterations,
        ),
    )

    s.addAlgorithm(algML)
    s.addAlgorithm(algGreedy)

    addTrackWriters(
        s,
        name="ambiML",
        tracks=algGreedy.config.outputTracks,
        outputDirCsv=outputDirCsv,
        outputDirRoot=outputDirRoot,
        writeStates=writeTrajectories,
        writeSummary=writeTrajectories,
        writeCKFperformance=True,
        writeFinderPerformance=False,
        writeFitterPerformance=False,
        logLevel=logLevel,
    )

    return s


@acts.examples.NamedTypeArgs(
    config=AmbiguityResolutionMLDBScanConfig,
)
def addAmbiguityResolutionMLDBScan(
    s,
    config: AmbiguityResolutionMLDBScanConfig = AmbiguityResolutionMLDBScanConfig(),
    onnxModelFile: Optional[Union[Path, str]] = None,
    outputDirCsv: Optional[Union[Path, str]] = None,
    outputDirRoot: Optional[Union[Path, str]] = None,
    writeTrajectories: bool = True,
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    from acts.examples import AmbiguityResolutionMLDBScanAlgorithm

    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    alg = AmbiguityResolutionMLDBScanAlgorithm(
        level=customLogLevel(),
        inputTracks="tracks",
        inputDuplicateNN=onnxModelFile,
        outputTracks="ambiTracksMLDBScan",
        **acts.examples.defaultKWArgs(
            nMeasurementsMin=config.nMeasurementsMin,
            epsilonDBScan=config.epsilonDBScan,
            minPointsDBScan=config.minPointsDBScan,
        ),
    )
    s.addAlgorithm(alg)

    addTrackWriters(
        s,
        name="ambiMLDBScan",
        trajectories=alg.config.outputTracks,
        outputDirRoot=outputDirRoot,
        outputDirCsv=outputDirCsv,
        writeStates=writeTrajectories,
        writeSummary=writeTrajectories,
        writeCKFperformance=True,
        writeFinderPerformance=False,
        writeFitterPerformance=False,
        logLevel=logLevel,
    )

    return s


@acts.examples.NamedTypeArgs(
    trackSelectorConfig=TrackSelectorConfig,
)
def addVertexFitting(
    s,
    field,
    tracks: Optional[str] = "tracks",
    trackParameters: Optional[str] = None,
    associatedParticles: Optional[str] = None,
    outputProtoVertices: str = "protovertices",
    outputVertices: str = "fittedVertices",
    seeder: Optional[acts.VertexSeedFinder] = acts.VertexSeedFinder.GaussianSeeder,
    vertexFinder: VertexFinder = VertexFinder.Truth,
    trackSelectorConfig: Optional[TrackSelectorConfig] = None,
    outputDirRoot: Optional[Union[Path, str]] = None,
    logLevel: Optional[acts.logging.Level] = None,
) -> None:
    """This function steers the vertex fitting

    Parameters
    ----------
    s: Sequencer
        the sequencer module to which we add the Seeding steps (returned from addVertexFitting)
    field : magnetic field
    outputDirRoot : Path|str, path, None
        the output folder for the Root output, None triggers no output
    associatedParticles : str, "associatedTruthParticles"
        VertexPerformanceWriter.inputAssociatedTruthParticles
    seeder : enum member
        determines vertex seeder, can be acts.seeder.GaussianSeeder or acts.seeder.AdaptiveGridSeeder
    vertexFinder : VertexFinder, Truth
        vertexFinder algorithm: one of Truth, AMVF, Iterative
    logLevel : acts.logging.Level, None
        logging level to override setting given in `s`
    """
    from acts.examples import (
        TruthVertexFinder,
        VertexFitterAlgorithm,
        IterativeVertexFinderAlgorithm,
        AdaptiveMultiVertexFinderAlgorithm,
        VertexPerformanceWriter,
    )

    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    if tracks is not None and trackSelectorConfig is not None:
        trackSelector = addTrackSelection(
            s,
            trackSelectorConfig,
            inputTracks=tracks,
            outputTracks="selectedTracksVertexing",
            logLevel=customLogLevel(),
        )
        tracks = trackSelector.config.outputTracks

    if trackParameters is None:
        converter = acts.examples.TracksToParameters(
            level=customLogLevel(),
            inputTracks=tracks,
            outputTrackParameters="selectedTracksParametersVertexing",
        )
        s.addAlgorithm(converter)
        trackParameters = converter.config.outputTrackParameters

    tracks = tracks if tracks is not None else ""
    associatedParticles = associatedParticles if associatedParticles is not None else ""
    inputParticles = "particles_input"
    selectedParticles = "particles_selected"

    if vertexFinder == VertexFinder.Truth:
        findVertices = TruthVertexFinder(
            level=customLogLevel(),
            inputParticles=selectedParticles,
            outputProtoVertices=outputProtoVertices,
            excludeSecondaries=True,
        )
        s.addAlgorithm(findVertices)
        fitVertices = VertexFitterAlgorithm(
            level=customLogLevel(),
            bField=field,
            inputTrackParameters=trackParameters,
            inputProtoVertices=findVertices.config.outputProtoVertices,
            outputVertices=outputVertices,
        )
        s.addAlgorithm(fitVertices)
    elif vertexFinder == VertexFinder.Iterative:
        findVertices = IterativeVertexFinderAlgorithm(
            level=customLogLevel(),
            bField=field,
            inputTrackParameters=trackParameters,
            outputProtoVertices=outputProtoVertices,
            outputVertices=outputVertices,
        )
        s.addAlgorithm(findVertices)
    elif vertexFinder == VertexFinder.AMVF:
        findVertices = AdaptiveMultiVertexFinderAlgorithm(
            level=customLogLevel(),
            bField=field,
            seedFinder=seeder,
            inputTrackParameters=trackParameters,
            outputProtoVertices=outputProtoVertices,
            outputVertices=outputVertices,
        )
        s.addAlgorithm(findVertices)
    else:
        raise RuntimeError("Invalid finder argument")

    if outputDirRoot is not None:
        outputDirRoot = Path(outputDirRoot)
        if not outputDirRoot.exists():
            outputDirRoot.mkdir()
        if associatedParticles == selectedParticles:
            warnings.warn(
                "Using VertexPerformanceWriter with smeared particles is not necessarily supported. "
                "Please get in touch with us"
            )
        s.addWriter(
            VertexPerformanceWriter(
                level=customLogLevel(),
                inputAllTruthParticles=inputParticles,
                inputSelectedTruthParticles=selectedParticles,
                inputMeasurementParticlesMap="measurement_particles_map",
                inputTracks=tracks,
                inputAssociatedTruthParticles=associatedParticles,
                inputVertices=outputVertices,
                bField=field,
                minTrackVtxMatchFraction=0.5 if associatedParticles else 0.0,
                treeName="vertexing",
                filePath=str(outputDirRoot / "performance_vertexing.root"),
            )
        )

    return s


def addSingleSeedVertexFinding(
    s,
    outputDirRoot: Optional[Union[Path, str]] = None,
    logLevel: Optional[acts.logging.Level] = None,
    inputSpacePoints: Optional[str] = "spacepoints",
    outputVertices: Optional[str] = "fittedSeedVertices",
) -> None:
    from acts.examples import (
        SingleSeedVertexFinderAlgorithm,
        VertexPerformanceWriter,
    )

    customLogLevel = acts.examples.defaultLogging(s, logLevel)

    findSingleSeedVertex = SingleSeedVertexFinderAlgorithm(
        level=customLogLevel(),
        inputSpacepoints=inputSpacePoints,
        outputVertices=outputVertices,
    )
    s.addAlgorithm(findSingleSeedVertex)

    inputParticles = "particles_input"
    selectedParticles = "particles_selected"

    if outputDirRoot is not None:
        outputDirRoot = Path(outputDirRoot)
        if not outputDirRoot.exists():
            outputDirRoot.mkdir()

        s.addWriter(
            VertexPerformanceWriter(
                level=customLogLevel(),
                inputAllTruthParticles=inputParticles,
                inputSelectedTruthParticles=selectedParticles,
                useTracks=False,
                inputVertices=outputVertices,
                treeName="seedvertexing",
                filePath=str(outputDirRoot / "performance_seedvertexing.root"),
            )
        )

    return s
