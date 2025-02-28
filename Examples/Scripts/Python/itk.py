#!/usr/bin/env python3
from pathlib import Path
import argparse

from acts.examples import (
    WhiteBoard,
    AlgorithmContext,
    ProcessCode,
    CsvTrackingGeometryWriter,
    ObjTrackingGeometryWriter,
    JsonSurfacesWriter,
    JsonMaterialWriter,
    JsonFormat,
)

import acts

from acts import MaterialMapJsonConverter


def runITk(
    trackingGeometry,
    decorators,
    outputDir: Path,
    events=1,
    outputObj=True,
    outputCsv=False,
    outputJson=False,
    material=True,
):
    for ievt in range(events):
        eventStore = WhiteBoard(name=f"EventStore#{ievt}", level=acts.logging.INFO)
        ialg = 0

        context = AlgorithmContext(ialg, ievt, eventStore)

        for cdr in decorators:
            r = cdr.decorate(context)
            if r != ProcessCode.SUCCESS:
                raise RuntimeError("Failed to decorate event context")

        if outputCsv:
            csv_dir = outputDir / "csv"
            csv_dir.mkdir(exist_ok=True)
            writer = CsvTrackingGeometryWriter(
                level=acts.logging.INFO,
                trackingGeometry=trackingGeometry,
                outputDir=str(csv_dir),
                writePerEvent=True,
            )
            writer.write(context)

        if outputObj:
            obj_dir = outputDir / "obj"
            obj_dir.mkdir(exist_ok=True)
            writer = ObjTrackingGeometryWriter(
                level=acts.logging.INFO,
                outputDir=str(obj_dir),
            )
            writer.write(context, trackingGeometry)

        if outputJson:
            json_dir = outputDir / "json"
            json_dir.mkdir(exist_ok=True)
            writer = JsonSurfacesWriter(
                level=acts.logging.INFO,
                trackingGeometry=trackingGeometry,
                outputDir=str(json_dir),
                writePerEvent=True,
                writeSensitive=True,
            )
            writer.write(context)

            jmConverterCfg = MaterialMapJsonConverter.Config(
                processSensitives=True,
                processApproaches=True,
                processRepresenting=True,
                processBoundaries=True,
                processVolumes=True,
                processNonMaterial=True,
                context=context.geoContext,
            )

            outname = "material-map"
            if not material:
                outname = "geometry-map"

            jmw = JsonMaterialWriter(
                level=acts.logging.VERBOSE,
                converterCfg=jmConverterCfg,
                fileName=str(json_dir / outname),
                writeFormat=JsonFormat.Json,
            )

            jmw.write(trackingGeometry)


if "__main__" == __name__:
    p = argparse.ArgumentParser(
        description="Example script to construct the ITk geometry and write it out to CSV and OBJ formats"
    )
    p.add_argument(
        "geo_dir",
        help="Input directory containing the ITk standalone geometry. Get in touch if you don't have this.",
    )
    p.add_argument(
        "--output-dir",
        default=Path.cwd(),
        type=Path,
        help="Directory to write outputs to",
    )
    p.add_argument(
        "--output-csv", action="store_true", help="Write geometry in CSV format."
    )
    p.add_argument(
        "--output-obj", action="store_true", help="Write geometry in OBJ format."
    )
    p.add_argument(
        "--output-json",
        action="store_true",
        help="Write geometry and material in JSON format.",
    )
    p.add_argument(
        "--no-material", action="store_true", help="Decorate material to the geometry"
    )

    args = p.parse_args()
    args.output_dir.mkdir(exist_ok=True, parents=True)

    geo_example_dir = Path(args.geo_dir)
    assert geo_example_dir.exists(), "Detector example input directory missing"
    from acts.examples.itk import buildITkGeometry

    detector = buildITkGeometry(
        geo_example_dir,
        material=not args.no_material,
    )
    trackingGeometry = detector.trackingGeometry()
    decorators = detector.contextDecorators()

    runITk(
        trackingGeometry=trackingGeometry,
        decorators=decorators,
        outputDir=args.output_dir,
        outputCsv=args.output_csv,
        outputObj=args.output_obj,
        outputJson=args.output_json,
        material=not args.no_material,
    )
