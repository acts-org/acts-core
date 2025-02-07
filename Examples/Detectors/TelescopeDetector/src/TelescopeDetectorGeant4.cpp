// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TelescopeDetector/TelescopeDetector.hpp"
#include "ActsExamples/TelescopeDetector/TelescopeG4DetectorConstruction.hpp"

namespace ActsExamples {

std::unique_ptr<G4VUserDetectorConstruction>
TelescopeDetector::buildGeant4DetectorConstruction(
    const Geant4ConstructionOptions& options) const {
  return std::make_unique<TelescopeG4DetectorConstruction>(m_cfg, options);
}

}  // namespace ActsExamples
