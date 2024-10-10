// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Geant4/EventStore.hpp"

#include <memory>
#include <string>
#include <vector>

#include <G4UserSteppingAction.hh>

class G4Step;

namespace ActsExamples {

/// @class MaterialSteppingAction
///
/// @brief Collects the RecordedMaterialSlab entities
///
/// The MaterialSteppingAction class is the implementation of the
/// Geant4 class MaterialSteppingAction. It extracts the weighted material
/// of every step and collects all material steps.
class MaterialSteppingAction final : public G4UserSteppingAction {
 public:
  /// Nested configuration struct
  struct Config {
    std::shared_ptr<EventStore> eventStore;

    std::vector<std::string> excludeMaterials = {};
  };

  /// Construct the action
  ///
  /// @param cfg the configuration struct for this Stepping action
  /// @param logger is an Acts::Logger for unique logging
  MaterialSteppingAction(const Config& cfg,
                         std::unique_ptr<const Acts::Logger> logger =
                             Acts::getDefaultLogger("SimParticleTranslation",
                                                    Acts::Logging::INFO));
  ~MaterialSteppingAction() override;

  /// @brief Action per step to be performed
  ///
  /// @param step is the Geant4 step of the particle
  void UserSteppingAction(const G4Step* step) override;

 private:
  /// Config struct
  Config m_cfg;

  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// Private access method to the event store
  EventStore& eventStore() const { return *m_cfg.eventStore; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
