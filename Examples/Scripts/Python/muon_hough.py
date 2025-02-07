#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import os
import argparse

import acts

from acts.examples import CsvMuonSimHitReader, CsvDriftCircleReader, MuonHoughSeeder

# from acts.examples.reconstruction import (
#     addSeeding,
#     addStandardSeeding,
#     SeedingAlgorithm,
# )

# from acts.examples.itk import itkSeedingAlgConfig, InputSpacePointsType

u = acts.UnitConstants
rnd = acts.examples.RandomNumbers(seed=42)


def runHoughFromCsv(inDir):
    # create temporary file with pixel SPs and run the seeding

    s = acts.examples.Sequencer(events=8, numThreads=1, logLevel=acts.logging.VERBOSE)

    # Read input space points from input csv files
    evReader = CsvMuonSimHitReader(
        inputStem="MuonSimHit",
        inputDir=os.path.dirname(inDir),
        outputSimHits="MuonSimHits",
        level=acts.logging.VERBOSE,
    )
    dcReader = CsvDriftCircleReader(
        inputStem="MuonDriftCircle",
        inputDir=os.path.dirname(inDir),
        outputDriftCircles="MuonDriftCircles",
        level=acts.logging.VERBOSE,
    )
    # add csv reader
    s.addReader(evReader)
    s.addReader(dcReader)
    seeder = MuonHoughSeeder(
        inSimHits=evReader.config.outputSimHits,
        inDriftCircles=dcReader.config.outputDriftCircles,
        level=acts.logging.VERBOSE,
    )
    s.addAlgorithm(seeder)
    s.run()


if "__main__" == __name__:
    p = argparse.ArgumentParser(
        description="Example script to run ITk seed finding based on CSV spacepoints",
    )
    p.add_argument(
        "indir",
        help="Input directory containing the ITk standalone geometry. Get in touch if you don't have this.",
    )

    args = p.parse_args()

    runHoughFromCsv(args.indir)
