# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

import os

import acts
import acts.examples.geant4 as actsG4

mockupConfig = actsG4.MockupSectorBuilder.Config()

mockupChamberConfigInner = actsG4.MockupSectorBuilder.ChamberConfig()
mockupChamberConfigInner.name = "Inner_Detector_Chamber"
mockupChamberConfigInner.SensitiveNames = ["Inner_Skin"]
mockupChamberConfigInner.PassiveNames = ["xx"]

mockupChamberConfigMiddle = actsG4.MockupSectorBuilder.ChamberConfig()
mockupChamberConfigMiddle.name = "Middle_Detector_Chamber"
mockupChamberConfigMiddle.SensitiveNames = ["Middle_Skin"]
mockupChamberConfigMiddle.PassiveNames = ["xx"]

mockupChamberConfigOuter = actsG4.MockupSectorBuilder.ChamberConfig()
mockupChamberConfigOuter.name = "Outer_Detector_Chamber"
mockupChamberConfigOuter.SensitiveNames = ["Outer_Skin"]
mockupChamberConfigOuter.PassiveNames = ["xx"]

dirOfThisScript = os.path.dirname(__file__)
mockupConfig.gdmlPath = os.path.join(
    dirOfThisScript,
    "../../../../Detectors/MuonSpectrometerMockupDetector/MuonChamber.gdml",
)
mockupConfig.NumberOfSectors = 8

mockupBuilder = actsG4.MockupSectorBuilder(mockupConfig)

detectorVolumeInner = mockupBuilder.buildChamber(mockupChamberConfigInner)

detectorVolumeOuter = mockupBuilder.buildChamber(mockupChamberConfigOuter)

detectorVolumeMiddle = mockupBuilder.buildChamber(mockupChamberConfigMiddle)

detectorVolumes = [detectorVolumeInner, detectorVolumeMiddle, detectorVolumeOuter]

detectorVolumeSector = mockupBuilder.buildSector(detectorVolumes)

mockupBuilder.drawSector(detectorVolumeSector, "sector_drawn_from_python")
