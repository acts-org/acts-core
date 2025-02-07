// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"
#include "ActsExamples/DD4hepDetector/DDG4DetectorConstruction.hpp"

#include <G4VUserDetectorConstruction.hh>

namespace ActsExamples {

std::unique_ptr<G4VUserDetectorConstruction>
DD4hepDetector::buildGeant4DetectorConstruction(
    const Geant4ConstructionOptions& options) const {
  return std::make_unique<DDG4DetectorConstruction>(m_detector, options);
}

}  // namespace ActsExamples
