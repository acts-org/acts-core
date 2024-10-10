// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <string>
#include <vector>

namespace ActsExamples {

/// Geant4 Region Creator
///
/// Used to set process cuts in specified volumes.
/// Particles will not be created if their energy is below the cut in length
/// units.
class RegionCreator {
 public:
  /// Nested configuration struct for the Geant4 region creator
  struct Config {
    /// Process cut to be applied for gammas, in mm
    double gammaCut{};

    /// Process cut to be applied for electrons, in mm
    double electronCut{};

    /// Process cut to be applied for positrons, in mm
    double positronCut{};

    /// Process cut to be applied for protons, in mm
    double protonCut{};

    /// Volume list to be included in this region
    std::vector<std::string> volumes{};
  };

  /// Region creator constructor
  ///
  /// @param cfg is the configuration struct
  /// @param name is the region name
  /// @param level is the logging level to be used
  RegionCreator(const Config& cfg, std::string name,
                Acts::Logging::Level level);

  /// Construct the region
  void Construct();

  /// Readonly access to the configuration
  const Config& config() const { return m_cfg; }

 private:
  /// Region name
  std::string m_name;

  /// Config instance
  Config m_cfg;

  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
