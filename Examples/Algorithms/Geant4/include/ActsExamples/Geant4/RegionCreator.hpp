// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <string>
#include <vector>

class G4Region;

namespace ActsExamples::Geant4 {

/// Geant4 Region Creator
///
/// Used to set process cuts in specified volumes.
/// Particles will not be created if their energy is below the cut in length
/// units.
class RegionCreator {
 public:
  /// Nested configuration struct for the Geant4 region creator
  struct Config {
    /// Region name
    std::string name;

    /// Process cut to be applied for gammas, in mm
    double gammaCut{};

    /// Process cut to be applied for electrons, in mm
    double electronCut{};

    /// Process cut to be applied for positrons, in mm
    double positronCut{};

    /// Process cut to be applied for protons, in mm
    double protonCut{};

    /// Volume list to be included in this region
    std::vector<std::string> volumes;
  };

  /// Region creator constructor
  ///
  /// @param cfg is the configuration struct
  explicit RegionCreator(const Config& cfg);

  /// Construct the region
  /// @note The lifetime of the returned region is managed by Geant4
  G4Region* buildRegion(
      const Acts::Logger& logger = Acts::getDummyLogger()) const;

  /// Readonly access to the configuration
  const Config& config() const { return m_cfg; }

 private:
  /// Config instance
  Config m_cfg;
};

}  // namespace ActsExamples::Geant4
