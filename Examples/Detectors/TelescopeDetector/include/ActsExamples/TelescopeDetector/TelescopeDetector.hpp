// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/DetectorCommons/Detector.hpp"

#include <array>
#include <memory>
#include <vector>

namespace Acts {
class IMaterialDecorator;
}  // namespace Acts

namespace ActsExamples {

class TelescopeDetector : public Detector {
 public:
  struct Config {
    std::vector<double> positions{{0, 30, 60, 120, 150, 180}};
    std::vector<double> stereos{{0, 0, 0, 0, 0, 0}};
    std::array<double, 2> offsets{{0, 0}};
    std::array<double, 2> bounds{{25, 100}};
    double thickness{80 * Acts::UnitConstants::um};
    int surfaceType{0};
    int binValue{2};
    std::shared_ptr<const Acts::IMaterialDecorator> materialDecorator;
    Acts::Logging::Level logLevel{Acts::Logging::WARNING};
  };

  explicit TelescopeDetector(const Config& cfg);

  std::unique_ptr<G4VUserDetectorConstruction> buildGeant4DetectorConstruction(
      const Geant4ConstructionOptions& options) const override;

 private:
  Config m_cfg;
};

}  // namespace ActsExamples
