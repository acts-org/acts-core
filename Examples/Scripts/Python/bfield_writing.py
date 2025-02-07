#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import acts
import acts.examples

u = acts.UnitConstants


def RootBFieldWrite(bField, fileName, treeName="solenoid", level=acts.logging.VERBOSE):
    cfg = acts.examples.RootBFieldWriter.Config()
    cfg.bField = bField
    cfg.gridType = acts.examples.RootBFieldWriter.GridType.rz
    cfg.fileName = str(fileName)
    cfg.treeName = treeName
    acts.examples.RootBFieldWriter.run(cfg, level)
    return cfg


def CsvBFieldWrite(bField, fileName, level=acts.logging.VERBOSE):
    cfg = acts.examples.CsvBFieldWriter.ConfigRzGrid()
    cfg.bField = bField
    cfg.fileName = str(fileName)
    acts.examples.CsvBFieldWriter.runRzGrid(cfg, level)
    return cfg


def runBFieldWriting(outputDir: Path, rewrites: int = 0):
    solenoid = acts.SolenoidBField(
        radius=1200 * u.mm, length=6000 * u.mm, bMagCenter=2 * u.T, nCoils=1194
    )
    field = acts.solenoidFieldMap(
        rlim=(0, 1200 * u.mm),
        zlim=(-5000 * u.mm, 5000 * u.mm),
        nbins=(10, 10),
        field=solenoid,
    )

    print("Solenoid ready")

    cfg = RootBFieldWrite(field, outputDir / "solenoid.root")
    CsvBFieldWrite(field, outputDir / "solenoid.csv")

    for i in range(rewrites):
        print(f"Now read back {cfg.fileName}")

        field2 = acts.examples.MagneticFieldMapRz(cfg.fileName, tree="solenoid")
        cfg2 = RootBFieldWrite(field2, outputDir / f"solenoid{i+2}.root")
        CsvBFieldWrite(field2, outputDir / f"solenoid{i+2}.csv")
        cfg = cfg2

    print("Done")


if "__main__" == __name__:
    runBFieldWriting(Path.cwd())
