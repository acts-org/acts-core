#!/usr/bin/env python3

# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

from pathlib import Path

import acts
from acts.examples import (
    readDigiConfigFromJson,
    DigitizationConfigurator,
    writeDigiConfigToJson,
    GenericDetector,
    DigiConfigContainer,
)


u = acts.UnitConstants


def runDigitizationConfig(
    trackingGeometry,
    input: Path,
    output: Path,
):
    inputConfig = readDigiConfigFromJson(str(input))

    digiConfigurator = DigitizationConfigurator()
    digiConfigurator.compactify = True
    digiConfigurator.inputDigiComponents = inputConfig

    trackingGeometry.visitSurfaces(digiConfigurator)

    outputConfig = DigiConfigContainer(digiConfigurator.outputDigiComponents)

    writeDigiConfigToJson(outputConfig, str(output))


if "__main__" == __name__:
    detector = GenericDetector()
    trackingGeometry = detector.trackingGeometry()

    runDigitizationConfig(
        trackingGeometry=trackingGeometry,
        input=Path(__file__).parent
        / "../../Algorithms/Digitization/share/default-smearing-config-generic.json",
        output=Path.cwd() / "digi-config-out.json",
    )
