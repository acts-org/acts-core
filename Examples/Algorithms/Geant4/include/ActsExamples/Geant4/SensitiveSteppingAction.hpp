// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>

#include <G4UserSteppingAction.hh>

namespace ActsExamples {

/// The G4SteppingAction that is called for every step in
/// the simulation process.
///
/// It checks whether a sensitive volume is present (via string tag)
/// and records (if necessary) the hit.
class SensitiveSteppingAction : public G4UserSteppingAction {
 public:
  /// Configuration of the Stepping action
  struct Config {
    /// Selection for hit recording
    bool charged = true;
    bool neutral = false;
    bool primary = true;
    bool secondary = false;
  };

  /// Construct the stepping action
  ///
  /// @param cfg the configuration struct
  /// @param logger the ACTS logging instance
  SensitiveSteppingAction(const Config& cfg,
                          std::unique_ptr<const Acts::Logger> logger =
                              Acts::getDefaultLogger("SensitiveSteppingAction",
                                                     Acts::Logging::INFO));
  ~SensitiveSteppingAction() override = default;

  /// @brief Interface Method doing the step and records the data
  /// @param step is the Geant4 step of the particle
  void UserSteppingAction(const G4Step* step) final override;

 protected:
  Config m_cfg;

 private:
  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
