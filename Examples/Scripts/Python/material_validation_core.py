#!/usr/bin/env python3

import os
import argparse
from pathlib import Path

import acts
from acts import (
    MaterialValidater,
    IntersectionMaterialAssigner,
    logging,
    GeometryContext,
    DetectorBuilder,
    GeometryIdGenerator,
)

from acts.examples import (
    Sequencer,
    WhiteBoard,
    AlgorithmContext,
    RootMaterialTrackWriter,
    MaterialValidation,
)


def runMaterialValidation(s, ntracks, surfaces, outputFile, seed, loglevel):
    # IO for material tracks reading
    wb = WhiteBoard(acts.logging.INFO)

    rnd = acts.examples.RandomNumbers(seed=seed)

    # Assignment setup : Intersection assigner
    materialAssingerConfig = IntersectionMaterialAssigner.Config()
    materialAssingerConfig.surfaces = surfaces
    materialAssinger = IntersectionMaterialAssigner(materialAssingerConfig, loglevel)

    # Validater setup
    materialValidaterConfig = MaterialValidater.Config()
    materialValidaterConfig.materialAssigner = materialAssinger
    materialValidater = MaterialValidater(materialValidaterConfig, loglevel)

    # Validation Algorithm
    materialValidationConfig = MaterialValidation.Config()
    materialValidationConfig.materialValidater = materialValidater
    materialValidationConfig.outputMaterialTracks = "recorded-material-tracks"
    materialValidationConfig.ntracks = ntracks
    materialValidationConfig.randomNumberSvc = rnd
    materialValidation = MaterialValidation(materialValidationConfig, loglevel)
    s.addAlgorithm(materialValidation)

    # Add the mapped material tracks writer
    s.addWriter(
        RootMaterialTrackWriter(
            level=acts.logging.INFO,
            inputMaterialTracks=materialValidationConfig.outputMaterialTracks,
            filePath=outputFile + ".root",
            storeSurface=True,
            storeVolume=True,
        )
    )
    # return the sequencer
    return s


if "__main__" == __name__:
    p = argparse.ArgumentParser()

    p.add_argument(
        "-n", "--events", type=int, default=1000, help="Number of events to process"
    )
    p.add_argument(
        "-t", "--tracks", type=int, default=100, help="Number of tracks per event"
    )
    p.add_argument(
        "-j", "--threads", type=int, default=-1, help="Number of threads in parallel"
    )
    p.add_argument(
        "-m", "--map", type=str, default="", help="Input file for the material map"
    )
    p.add_argument("-o", "--output", type=str, default="", help="Output file name")

    p.add_argument(
        "--experimental",
        action=argparse.BooleanOptionalAction,
        help="Construct experimental geometry",
    )

    p.add_argument(
        "--geomodel-input",
        type=str,
        default="",
        help="Construct experimental geometry from GeoModel",
    )

    p.add_argument(
        "--geomodel-top-node",
        type=str,
        default="",
        help="Top node definition of the GeoModel tree",
    )

    p.add_argument(
        "--geomodel-table-name",
        type=str,
        default="ActsBlueprint",
        help="Name of the blueprint table",
    )

    p.add_argument(
        "--geomodel-queries",
        nargs="+",
        type=str,
        default=[],
        help="Queries for published GeoModel nodes",
    )

    args = p.parse_args()
    gContext = GeometryContext()
    logLevel = acts.logging.INFO

    materialDecorator = None
    if args.map != "":
        materialDecorator = acts.IMaterialDecorator.fromFile(args.map)

    if args.experimental:
        if len(args.geomodel_input) > 0:
            from acts import geomodel as gm

            # Read the geometry model from the database
            gmTree = acts.geomodel.readFromDb(args.geomodel_input)

            gmFactoryConfig = gm.GeoModelDetectorSurfaceFactory.Config()
            gmFactoryConfig.shapeConverters = [
                gm.GeoBoxConverter(),
                gm.GeoTrdConverter(),
                gm.GeoIntersectionAnnulusConverter(),
            ]

            gmFactory = gm.GeoModelDetectorSurfaceFactory(gmFactoryConfig, logLevel)
            # The options
            gmFactoryOptions = gm.GeoModelDetectorSurfaceFactory.Options()
            gmFactoryOptions.queries = args.geomodel_queries
            # The Cache & construct call
            gmFactoryCache = gm.GeoModelDetectorSurfaceFactory.Cache()
            gmFactory.construct(gmFactoryCache, gContext, gmTree, gmFactoryOptions)

            # All surfaces from GeoModel
            gmSurfaces = [ss[1] for ss in gmFactoryCache.sensitiveSurfaces]

            # Construct the building hierarchy
            gmBlueprintConfig = gm.GeoModelBlueprintCreater.Config()
            gmBlueprintConfig.detectorSurfaces = gmSurfaces
            gmBlueprintConfig.kdtBinning = [acts.Binning.z, acts.Binning.r]

            gmBlueprintOptions = gm.GeoModelBlueprintCreater.Options()
            gmBlueprintOptions.table = args.geomodel_table_name
            gmBlueprintOptions.topEntry = args.geomodel_top_node
            gmBlueprintCreater = gm.GeoModelBlueprintCreater(
                gmBlueprintConfig, logLevel
            )
            gmBlueprint = gmBlueprintCreater.create(
                gContext, gmTree, gmBlueprintOptions
            )

            gmCylindricalBuilder = gmBlueprint.convertToBuilder(logLevel)

            # Top level geo id generator
            gmGeoIdConfig = GeometryIdGenerator.Config()
            gmGeoIdGenerator = GeometryIdGenerator(
                gmGeoIdConfig, "GeoModelGeoIdGenerator", logging.INFO
            )

            # Create the detector builder
            gmDetectorConfig = DetectorBuilder.Config()
            gmDetectorConfig.name = args.geomodel_top_node + "_DetectorBuilder"
            gmDetectorConfig.builder = gmCylindricalBuilder
            gmDetectorConfig.geoIdGenerator = gmGeoIdGenerator
            gmDetectorConfig.materialDecorator = materialDecorator
            gmDetectorConfig.auxiliary = (
                "GeoModel based Acts::Detector from '" + args.geomodel_input + "'"
            )

            gmDetectorBuilder = DetectorBuilder(
                gmDetectorConfig, args.geomodel_top_node, logLevel
            )
            detector = gmDetectorBuilder.construct(gContext)

            materialSurfaces = detector.extractMaterialSurfaces()
        else:
            from acts.examples.dd4hep import (
                DD4hepDetector,
                DD4hepDetectorOptions,
                DD4hepGeometryService,
            )

            from acts.examples.odd import (
                getOpenDataDetector,
                getOpenDataDetectorDirectory,
            )

            odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"

            # Create the dd4hep geometry service and detector
            dd4hepConfig = DD4hepGeometryService.Config()
            dd4hepConfig.logLevel = acts.logging.INFO
            dd4hepConfig.xmlFileNames = [str(odd_xml)]
            dd4hepGeometryService = DD4hepGeometryService(dd4hepConfig)
            dd4hepDetector = DD4hepDetector(dd4hepGeometryService)

            cOptions = DD4hepDetectorOptions(
                logLevel=acts.logging.INFO, emulateToGraph=""
            )
            cOptions.materialDecorator = materialDecorator

            # Context and options
            geoContext = acts.GeometryContext()
            [detector, contextors, store] = dd4hepDetector.finalize(
                geoContext, cOptions
            )

            materialSurfaces = detector.extractMaterialSurfaces()

    else:
        [detector, trackingGeometry, decorators] = getOpenDataDetector(
            materialDecorator
        )

        materialSurfaces = trackingGeometry.extractMaterialSurfaces()

    s = acts.examples.Sequencer(events=args.events, numThreads=args.threads)

    runMaterialValidation(
        s, args.tracks, materialSurfaces, args.output, 42, acts.logging.INFO
    ).run()
