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
#include <optional>
#include <string>

#include <G4VUserPrimaryGeneratorAction.hh>
#include <globals.hh>

class G4Event;

namespace ActsExamples {

/// @class SimParticleTranslation
///
/// Translates the Generated SimParticles from ACTS into
/// Geant4 particles and acts as the associated Geant4 primary
/// generator action.
///
/// This also ensures that the event numbers correspond to the
/// ACTS framework event numbers and hence harmonizes the EventStore.
class SimParticleTranslation final : public G4VUserPrimaryGeneratorAction {
 public:
  /// Nested configuration struct that contains the
  /// input particle collection name,
  struct Config {
    std::shared_ptr<EventStore> eventStore;

    /// Force pdgCode & mass & charge in G4 units (this is needed for Geantino
    /// simulation)
    std::optional<G4int> forcedPdgCode;
    std::optional<G4double> forcedCharge;  // e.g. 1 for charged geantino
    std::optional<G4double> forcedMass;    // e.g. 0 for geantino

    /// The number of hits per particle to be expected
    /// @note best to include secondaries for that
    unsigned int reserveHitsPerParticle = 20;
  };

  /// Construct the generator action
  ///
  /// @param cfg the configuration struct
  /// @param logger the ACTS logging instance
  SimParticleTranslation(const Config& cfg,
                         std::unique_ptr<const Acts::Logger> logger =
                             Acts::getDefaultLogger("SimParticleTranslation",
                                                    Acts::Logging::INFO));

  ~SimParticleTranslation() override;

  /// Interface method to generate the primary
  ///
  /// @param anEvent is the event that will be run
  void GeneratePrimaries(G4Event* anEvent) override;

 protected:
  Config m_cfg;

 private:
  /// Event number cache for EventStore harmonization
  unsigned int m_eventNr = 0;

  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// Private access method to the event store
  EventStore& eventStore() const { return *m_cfg.eventStore; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
