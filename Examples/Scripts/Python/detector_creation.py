#!/usr/bin/env python3

import json

import acts
from acts.examples.dd4hep import (
    DD4hepDetector,
    DD4hepDetectorOptions,
    DD4hepGeometryService,
)
from acts.examples.odd import getOpenDataDetectorDirectory


if "__main__" == __name__:
    odd_xml = getOpenDataDetectorDirectory() / "xml" / "OpenDataDetector.xml"

    print("Using the following xml file: ", odd_xml)

    # Create the dd4hep geometry service and detector
    dd4hepConfig = DD4hepGeometryService.Config()
    dd4hepConfig.logLevel = acts.logging.INFO
    dd4hepConfig.xmlFileNames = [str(odd_xml)]
    dd4hepGeometryService = DD4hepGeometryService(dd4hepConfig)
    dd4hepDetector = DD4hepDetector(dd4hepGeometryService)

    cOptions = DD4hepDetectorOptions(logLevel=acts.logging.INFO, emulateToGraph="")

    # Uncomment if you want to use the geometry id mapping
    # This map can be produced with the 'geometry.py' script
    geoIdMappingFile = None  # "odd-dd4hep-geoid-mapping-wo-extra.json"
    if geoIdMappingFile is not None:
        # Load the geometry id mapping json file
        with open(geoIdMappingFile) as f:
            # load the file as is
            geometry_id_mapping = json.load(f)
            # create a dictionary with GeometryIdentifier as value
            geometry_id_mapping_patched = {
                int(k): acts.GeometryIdentifier(int(v))
                for k, v in geometry_id_mapping.items()
            }
            # patch the options struct
            acts.examples.dd4hep.attachDD4hepGeoIdMapper(
                cOptions, geometry_id_mapping_patched
            )

    # Context and options
    geoContext = acts.GeometryContext()
    [detector, contextors, store] = dd4hepDetector.finalize(geoContext, cOptions)

    # OBJ style output
    surfaces = []
    viewConfig = acts.ViewConfig()
    viewConfig.nSegments = 100
    for vol in detector.volumePtrs():
        for surf in vol.surfacePtrs():
            if surf.geometryId().sensitive() > 0:
                surfaces.append(surf)
    acts.examples.writeSurfacesObj(surfaces, geoContext, viewConfig, "odd-surfaces.obj")

    # SVG style output
    surfaceStyle = acts.svg.Style()
    surfaceStyle.fillColor = [5, 150, 245]
    surfaceStyle.fillOpacity = 0.5

    surfaceOptions = acts.svg.SurfaceOptions()
    surfaceOptions.style = surfaceStyle

    viewRange = acts.Extent([])
    volumeOptions = acts.svg.DetectorVolumeOptions()
    volumeOptions.surfaceOptions = surfaceOptions

    # Transverse view
    xyRange = acts.Extent([[acts.BinningValue.binZ, [-50, 50]]])
    xyView = acts.svg.drawDetector(
        geoContext,
        detector,
        "odd",
        [[ivol, volumeOptions] for ivol in range(detector.numberVolumes())],
        [["xy", ["sensitives"], xyRange]],
    )
    xyFile = acts.svg.file()
    xyFile.addObjects(xyView)
    xyFile.write("odd_xy.svg")

    # Longitudinal view
    zrRange = acts.Extent([[acts.BinningValue.binPhi, [-0.1, 0.1]]])
    zrView = acts.svg.drawDetector(
        geoContext,
        detector,
        "odd",
        [[ivol, volumeOptions] for ivol in range(detector.numberVolumes())],
        [["zr", ["sensitives", "portals"], zrRange]],
    )
    zrFile = acts.svg.file()
    zrFile.addObjects(zrView)
    zrFile.write("odd_zr.svg")
