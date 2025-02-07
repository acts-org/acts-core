// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/GeoModelDetector/GeoModelDetector.hpp"
#include "ActsExamples/GeoModelDetector/GeoModelGeant4DetectorConstruction.hpp"

namespace ActsExamples {

std::unique_ptr<G4VUserDetectorConstruction>
GeoModelDetector::buildGeant4DetectorConstruction(
    const Geant4ConstructionOptions& options) const {
  return std::make_unique<GeoModelGeant4DetectorConstruction>(m_geoModel,
                                                              options);
}

}  // namespace ActsExamples
