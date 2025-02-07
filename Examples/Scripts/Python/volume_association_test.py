#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import acts
import argparse
import acts.examples
import acts.examples.odd_light as odd_light
from acts.examples import geant4 as acts_g4

from acts import GeometryContext, logging


def volumeAssociationTest(sequencer, ntests, tdetector):
    rnd = acts.examples.RandomNumbers(seed=42)

    alg = acts.examples.VolumeAssociationTest(
        name="VolumeAssociation",
        ntests=ntests,
        detector=tdetector,
        randomNumbers=rnd,
        randomRange=[1100, 3100],
        level=logging.DEBUG,
    )
    # Add the algorithm to the sequenceer
    sequencer.addAlgorithm(alg)
    return sequencer


def main():
    # Parse the command line arguments
    p = argparse.ArgumentParser()
    p.add_argument(
        "-i",
        "--input",
        type=str,
        default="odd-light.gdml",
        help="GDML input file (optional)",
    )
    p.add_argument(
        "-s",
        "--sensitives",
        type=str,
        default="phys_vol",
        help="Match string for sensitive surfaces",
    )
    p.add_argument(
        "-p",
        "--passives",
        type=str,
        default="pass_vol",
        help="Match string for passive surfaces",
    )
    p.add_argument(
        "-n", "--events", type=int, default=1000, help="Number of events to generate"
    )
    p.add_argument("-t", "--tests", type=int, default=10000, help="Tests per track")

    args = p.parse_args()
    geoContext = GeometryContext()

    # Convert the detector surfaces from GDML
    [_, ssurfaces, psurfaces] = acts_g4.convertSurfaces(
        args.input, [args.sensitives], [args.passives]
    )
    odd = odd_light.get_detector(geoContext, ssurfaces, psurfaces, logging.INFO)
    seq = acts.examples.Sequencer(events=args.events, numThreads=1)
    volumeAssociationTest(seq, args.tests, odd).run()


if "__main__" == __name__:
    main()
