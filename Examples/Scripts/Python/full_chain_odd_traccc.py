#!/usr/bin/env python3

import os
import argparse
import pathlib

import acts
import acts.examples
from acts.examples.simulation import (
    addParticleGun,
    MomentumConfig,
    EtaConfig,
    PhiConfig,
    ParticleConfig,
    addPythia8,
    addFatras,
    addGeant4,
    ParticleSelectorConfig,
    addDigitization,
    addParticleSelection,
)

from acts.examples.dd4hep import (
    DD4hepDetector,
    DD4hepDetectorOptions,
    DD4hepGeometryService,
)

from acts.examples.reconstruction import (
    addTracccChain,
)

from acts.examples.odd import getOpenDataDetector, getOpenDataDetectorDirectory

u = acts.UnitConstants

parser = argparse.ArgumentParser(description="Sim-digi chain with the OpenDataDetector")
parser.add_argument(
    "--output",
    "-o",
    help="Output directory",
    type=pathlib.Path,
    default=pathlib.Path.cwd() / "traccc_output",
)
parser.add_argument("--events", "-n", help="Number of events", type=int, default=100)
parser.add_argument("--skip", "-s", help="Number of events", type=int, default=0)
parser.add_argument(
    "--geant4", help="Use Geant4 instead of fatras", action="store_true"
)
parser.add_argument(
    "--ttbar",
    help="Use Pythia8 (ttbar, pile-up 200) instead of particle gun",
    action="store_true",
)
parser.add_argument(
    "--ttbar-pu",
    help="Number of pile-up events for ttbar",
    type=int,
    default=200,
)
parser.add_argument(
    "--gun-particles",
    help="Particle multiplicity of the particle gun",
    type=int,
    default=200,
)
parser.add_argument(
    "--gun-vertices",
    help="Vertex mulitplicity for the particle gun",
    type=int,
    default=1,
)
parser.add_argument(
    "--gun-eta-range",
    nargs="+",
    help="Eta range of the particle gun",
    type=float,
    default=[-3.0, 3.0],
)
parser.add_argument(
    "--gun-pt-range",
    nargs="+",
    help="Pt range of the particle gun (GeV)",
    type=float,
    default=[0.1 * u.GeV, 100 * u.GeV],
)
parser.add_argument(
    "--rnd-seed",
    help="Random seed",
    type=int,
    default=42,
)
parser.add_argument(
    "--digi-config",
    help="Digitization configuration file",
    type=str,
    default="",
)

parser.add_argument(
    "--mat-map",
    help="Material map file",
    type=str,
    default="",
)

parser.add_argument(
    "--experimental",
    action=argparse.BooleanOptionalAction,
    help="Construct experimental geometry",
)

parser.add_argument(
    "--platform",
    help="Platform to run the traccc chain on (host/device)",
    type=str,
    default="host"
)

args = parser.parse_args()

decorators = None
if args.mat_map != "":
    decorators = acts.IMaterialDecorator.fromFile(args.map)

if args.experimental:
    odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"

    # Create the dd4hep geometry service and detector
    dd4hepConfig = DD4hepGeometryService.Config()
    dd4hepConfig.logLevel = acts.logging.INFO
    dd4hepConfig.xmlFileNames = [str(odd_xml)]
    dd4hepGeometryService = DD4hepGeometryService(dd4hepConfig)
    detector = DD4hepDetector(dd4hepGeometryService)

    cOptions = DD4hepDetectorOptions(logLevel=acts.logging.INFO, emulateToGraph="")
    cOptions.materialDecorator = decorators

    # Context and options
    geoContext = acts.GeometryContext()
    [recoGeometry, contextors, store] = detector.finalize(geoContext, cOptions)


else:
    [detector, recoGeometry, decorators] = getOpenDataDetector(decorators)


outputDir = args.output
oddDigiConfig = args.digi_config
# Field specifications
field = acts.ConstantBField(acts.Vector3(0.0, 0.0, 2.0 * u.T))

rnd = acts.examples.RandomNumbers(seed=args.rnd_seed)

s = acts.examples.Sequencer(
    events=args.events,
    skip=args.skip,
    numThreads=1 if args.geant4 else -1,
    outputDir=str(outputDir),
    trackFpes=False,
)


if not args.ttbar:
    addParticleGun(
        s,
        MomentumConfig(
            args.gun_pt_range[0] * u.GeV,
            args.gun_pt_range[1] * u.GeV,
            transverse=True,
        ),
        EtaConfig(args.gun_eta_range[0], args.gun_eta_range[1]),
        PhiConfig(0.0, 360.0 * u.degree),
        ParticleConfig(
            args.gun_particles, acts.PdgParticle.eMuon, randomizeCharge=True
        ),
        vtxGen=acts.examples.GaussianVertexGenerator(
            mean=acts.Vector4(0, 0, 0, 0),
            stddev=acts.Vector4(0.0125 * u.mm, 0.0125 * u.mm, 55.5 * u.mm, 1.0 * u.ns),
        ),
        multiplicity=args.gun_vertices,
        rnd=rnd,
    )
else:
    addPythia8(
        s,
        hardProcess=["Top:qqbar2ttbar=on"],
        npileup=args.ttbar_pu,
        vtxGen=acts.examples.GaussianVertexGenerator(
            mean=acts.Vector4(0, 0, 0, 0),
            stddev=acts.Vector4(0.0125 * u.mm, 0.0125 * u.mm, 55.5 * u.mm, 5.0 * u.ns),
        ),
        rnd=rnd,
        outputDirRoot=outputDir,
        # outputDirCsv=outputDir,
    )

if args.geant4:
    if s.config.numThreads != 1:
        raise ValueError("Geant 4 simulation does not support multi-threading")

    # Pythia can sometime simulate particles outside the world volume, a cut on the Z of the track help mitigate this effect
    # Older version of G4 might not work, this as has been tested on version `geant4-11-00-patch-03`
    # For more detail see issue #1578
    addGeant4(
        s,
        detector,
        recoGeometry,
        field,
        preSelectParticles=ParticleSelectorConfig(
            rho=(0.0, 24 * u.mm),
            absZ=(0.0, 1.0 * u.m),
            eta=(-3.0, 3.0),
            pt=(150 * u.MeV, None),
            removeNeutral=True,
        ),
        outputDirRoot=outputDir,
        outputDirCsv=outputDir,
        rnd=rnd,
        killVolume=(
            recoGeometry.worldVolume
            if isinstance(recoGeometry, acts.TrackingGeometry)
            else None
        ),
        killAfterTime=25 * u.ns,
    )
elif isinstance(recoGeometry, acts.TrackingGeometry):
    addFatras(
        s,
        recoGeometry,
        field,
        preSelectParticles=(
            ParticleSelectorConfig(
                rho=(0.0, 24 * u.mm),
                absZ=(0.0, 1.0 * u.m),
                eta=(-3.0, 3.0),
                pt=(150 * u.MeV, None),
                removeNeutral=True,
            )
            if args.ttbar
            else ParticleSelectorConfig()
        ),
        enableInteractions=True,
        outputDirRoot=outputDir,
        outputDirCsv=outputDir,
        rnd=rnd,
    )
else:
    raise ValueError("Fatras simulation with Experimental::Detector not yet supported")

addDigitization(
    s,
    recoGeometry,
    field,
    digiConfigFile=oddDigiConfig,
    outputDirRoot=outputDir,
    outputDirCsv=outputDir,
    rnd=rnd,
    minEnergyDeposit=0,
    doMerge=True,
    mergeCommonCorner=True,
)

chainConfig = acts.examples.TracccChainConfig()

addTracccChain(
    s,
    recoGeometry,
    field,
    digiConfigFile=oddDigiConfig,
    inputCells="cells",
    outputDirRoot=outputDir,
    chainConfig=chainConfig,
    logLevel=acts.logging.DEBUG,
    platform=args.platform,
    # outputDirCsv=outputDir,
)

s.run()
