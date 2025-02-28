#!/usr/bin/env python3
import os
import argparse
import tempfile

import pathlib, acts

from acts.examples import (
    CsvSpacePointReader,
    TrackParamsEstimationAlgorithm,
    SeedingPerformanceWriter,
)
from acts.examples.reconstruction import (
    addStandardSeeding,
)

from acts.examples.itk import itkSeedingAlgConfig, InputSpacePointsType

u = acts.UnitConstants
rnd = acts.examples.RandomNumbers(seed=42)


def runITkSeedingFromCsv(detector, trackingGeometry, field, outputDir):
    # create temporary file with pixel SPs and run the seeding
    with tempfile.TemporaryDirectory() as tmpdirname:
        temp = open(tmpdirname + "/event000000000-spacepoints_pixel.csv", "w+t")
        print(
            "created temporary file: "
            + tmpdirname
            + "/event000000000-spacepoints_pixel.csv"
        )
        temp.write(
            "measurement_id,sp_type,module_idhash,sp_x,sp_y,sp_z,sp_radius,sp_covr,sp_covz,sp_topHalfStripLength,sp_bottomHalfStripLength,sp_topStripDirection[0],sp_topStripDirection[1],sp_topStripDirection[2],sp_bottomStripDirection[0],sp_bottomStripDirection[1],sp_bottomStripDirection[2],sp_stripCenterDistance[0],sp_stripCenterDistance[1],sp_stripCenterDistance[2],sp_topStripCenterPosition[0],sp_topStripCenterPosition[1],sp_topStripCenterPosition[2]\n 1,0,3139,32.67557144165039,-5.311902523040771,-47.65000152587891,33.10452270507812,0.05999999865889549,0.02999880164861679,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n 2,0,3422,95.14442443847656,-15.46361255645752,-52.125,96.39286804199219,0.05999999865889549,0.01687432639300823,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n 3,0,3650,102.8257064819336,-16.71612739562988,-52.67499923706055,104.1755981445312,0.05999999865889549,0.001875000074505806,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n 4,0,4223,159.4266204833984,-25.91166687011719,-56.75,161.5186157226562,0.05999999865889549,0.02999880164861679,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n 5,0,5015,224.07958984375,-36.37123107910156,-61.40000152587891,227.0121765136719,0.05999999865889549,0.007499700412154198,0,0,0,0,0,0,0,0,0,0,0,0,0,0\n 6,0,6023,284.1485595703125,-46.0638542175293,-65.72499847412109,287.8580932617188,0.05999999865889549,0.001875000074505806,0,0,0,0,0,0,0,0,0,0,0,0,0,0"
        )
        temp.read()

        s = acts.examples.Sequencer(events=1, numThreads=-1, logLevel=acts.logging.INFO)

        # Read input space points from input csv files
        evReader = CsvSpacePointReader(
            level=acts.logging.INFO,
            inputStem="spacepoints",
            inputCollection="pixel",
            inputDir=os.path.dirname(temp.name),
            outputSpacePoints="PixelSpacePoint",
            extendCollection=False,
        )

        # add csv reader
        s.addReader(evReader)
        spacePoints = evReader.config.outputSpacePoints

        # run seeding
        inputSeeds = addStandardSeeding(
            s,
            spacePoints,
            *acts.examples.itk.itkSeedingAlgConfig(
                InputSpacePointsType.PixelSpacePoints
            ),
        )

        prototracks = "seed-prototracks"
        s.addAlgorithm(
            acts.examples.SeedsToPrototracks(
                level=acts.logging.INFO,
                inputSeeds=inputSeeds,
                outputProtoTracks=prototracks,
            )
        )

        # estimate seeding performance
        parEstimateAlg = TrackParamsEstimationAlgorithm(
            level=acts.logging.INFO,
            inputSeeds=inputSeeds,
            outputTrackParameters="estimatedparameters",
            trackingGeometry=trackingGeometry,
            magneticField=field,
        )
        s.addAlgorithm(parEstimateAlg)

        s.run()

    # create temporary file with strips SPs and run the seeding
    with tempfile.TemporaryDirectory() as tmpdirname:
        temp = open(tmpdirname + "/event000000000-spacepoints_strip.csv", "w+t")
        print(
            "created temporary file: "
            + tmpdirname
            + "/event000000000-spacepoints_strip.csv"
        )
        temp.write(
            "measurement_id,sp_type,module_idhash,sp_x,sp_y,sp_z,sp_radius,sp_covr,sp_covz,sp_topHalfStripLength,sp_bottomHalfStripLength,sp_topStripDirection[0],sp_topStripDirection[1],sp_topStripDirection[2],sp_bottomStripDirection[0],sp_bottomStripDirection[1],sp_bottomStripDirection[2],sp_stripCenterDistance[0],sp_stripCenterDistance[1],sp_stripCenterDistance[2],sp_topStripCenterPosition[0],sp_topStripCenterPosition[1],sp_topStripCenterPosition[2]\n 0,1,0,386.77178955078125,-62.579288482666015625,-72.66841888427734375,391.801727294921875,0.100000001490116119384765625,5.11999988555908203125,12.08999919891357421875,12.08999919891357421875,-0.00864744372665882110595703125,-0.02451671846210956573486328125,0.999662101268768310546875,0.00864744372665882110595703125,0.02451671846210956573486328125,0.999662101268768310546875,-6.43960094451904296875,1.04346692562103271484375,23.157070159912109375,386.6771240234375,-62.847682952880859375,-61.724697113037109375\n 1,1,0,543.9947509765625,-87.7279205322265625,-82.09113311767578125,551.02313232421875,0.100000001490116119384765625,5.11999988555908203125,12.08999919891357421875,12.08999919891357421875,-0.0073835677467286586761474609375,-0.024926505982875823974609375,0.999662101268768310546875,0.0073835677467286586761474609375,0.024926505982875823974609375,0.999662101268768310546875,-6.34883975982666015625,1.17108881473541259765625,-2.3926274776458740234375,544.0279541015625,-87.6157989501953125,-86.58742523193359375\n 2,1,0,544.00189208984375,-87.70365142822265625,-83.064239501953125,551.02630615234375,0.100000001490116119384765625,5.11999988555908203125,12.08999919891357421875,12.08999919891357421875,-0.0073835677467286586761474609375,-0.024926505982875823974609375,0.999662101268768310546875,0.011192028410732746124267578125,0.02346457540988922119140625,0.999662101268768310546875,-24.83704376220703125,4.08867168426513671875,-0.0274789035320281982421875,544.0279541015625,-87.6157989501953125,-86.58742523193359375\n 3,1,0,562.2547607421875,-90.650543212890625,-83.99970245361328125,569.5155029296875,0.100000001490116119384765625,5.11999988555908203125,12.08999919891357421875,12.08999919891357421875,-0.011192028410732746124267578125,-0.02346457540988922119140625,0.999662101268768310546875,0.0073835677467286586761474609375,0.024926505982875823974609375,0.999662101268768310546875,11.8808460235595703125,-1.85768926143646240234375,-0.0588833652436733245849609375,562.25762939453125,-90.6445770263671875,-84.2536773681640625\n 4,1,0,562.26605224609375,-90.62686920166015625,-85.00818634033203125,569.52288818359375,0.100000001490116119384765625,5.11999988555908203125,12.08999919891357421875,12.08999919891357421875,-0.011192028410732746124267578125,-0.02346457540988922119140625,0.999662101268768310546875,0.011192028410732746124267578125,0.02346457540988922119140625,0.999662101268768310546875,-6.60735797882080078125,1.05989348888397216796875,2.3062651157379150390625,562.25762939453125,-90.6445770263671875,-84.2536773681640625\n 5,1,0,750.5875244140625,-120.5648040771484375,-98.9385986328125,760.2088623046875,0.100000001490116119384765625,5.11999988555908203125,24.1799983978271484375,24.1799983978271484375,-0.009588238783180713653564453125,-0.02416430227458477020263671875,0.999662101268768310546875,0.009588238783180713653564453125,0.02416430227458477020263671875,0.999662101268768310546875,-6.041371822357177734375,2.1813886165618896484375,0.3670396506786346435546875,750.81573486328125,-119.98968505859375,-122.7309112548828125\n 6,1,0,979.34979248046875,-156.5580291748046875,-114.63397979736328125,991.78448486328125,0.100000001490116119384765625,5.11999988555908203125,24.1799983978271484375,24.1799983978271484375,-0.00824898667633533477783203125,-0.02465364150702953338623046875,0.999662101268768310546875,0.00824898667633533477783203125,0.02465364150702953338623046875,0.999662101268768310546875,-6.2955722808837890625,1.417438507080078125,-1.45441913604736328125,979.4241943359375,-156.335723876953125,-123.64752960205078125"
        )
        temp.read()

        s = acts.examples.Sequencer(events=1, numThreads=-1, logLevel=acts.logging.INFO)

        # Read input space points from input csv files
        evReader = CsvSpacePointReader(
            level=acts.logging.INFO,
            inputStem="spacepoints",
            inputCollection="strip",
            inputDir=os.path.dirname(temp.name),
            outputSpacePoints="StripSpacePoint",
            extendCollection=True,
        )

        # add csv reader
        s.addReader(evReader)
        spacePoints = evReader.config.outputSpacePoints

        # run seeding
        inputSeeds = addStandardSeeding(
            s,
            spacePoints,
            *acts.examples.itk.itkSeedingAlgConfig(
                InputSpacePointsType.StripSpacePoints
            ),
        )

        prototracks = "seed-prototracks"
        s.addAlgorithm(
            acts.examples.SeedsToPrototracks(
                level=acts.logging.INFO,
                inputSeeds=inputSeeds,
                outputProtoTracks=prototracks,
            )
        )

        # estimate seeding performance
        parEstimateAlg = TrackParamsEstimationAlgorithm(
            level=acts.logging.INFO,
            inputSeeds=inputSeeds,
            outputTrackParameters="estimatedparameters",
            trackingGeometry=trackingGeometry,
            magneticField=field,
        )
        s.addAlgorithm(parEstimateAlg)

        s.run()


if "__main__" == __name__:
    p = argparse.ArgumentParser(
        description="Example script to run ITk seed finding based on CSV spacepoints",
    )

    geo_dir = pathlib.Path("acts-itk")
    outputDir = pathlib.Path.cwd() / "itk_output"

    detector = acts.examples.itk.buildITkGeometry(geo_dir)
    trackingGeometry = detector.trackingGeometry()
    field = acts.examples.MagneticFieldMapXyz(
        str(geo_dir / "bfield/ATLAS-BField-xyz.root")
    )

    runITkSeedingFromCsv(detector, trackingGeometry, field, outputDir)
