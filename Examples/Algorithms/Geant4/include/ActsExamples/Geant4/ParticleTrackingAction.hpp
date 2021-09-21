// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

#include <memory>
#include <string>

#include <G4Track.hh>
#include <G4UserTrackingAction.hh>

namespace ActsExamples {

/// The G4UserTrackingAction that is called for every track in
/// the simulation process.
///
/// It records the initial and final particle state
class ParticleTrackingAction : public G4UserTrackingAction {
 public:
  struct Config {};

  /// Construct the stepping action
  ///
  /// @param cfg the configuration struct
  /// @param logger the ACTS logging instance
  ParticleTrackingAction(const Config& cfg,
                         std::unique_ptr<const Acts::Logger> logger =
                             Acts::getDefaultLogger("ParticleTrackingAction",
                                                    Acts::Logging::INFO));
  ~ParticleTrackingAction() override = default;

  /// Action before the track is processed in the
  /// the simulation, this will record the initial particle
  ///
  /// @param aTrack the current Geant4 track
  void PreUserTrackingAction(const G4Track* aTrack) final;

  /// Action after the track is processed in the
  /// the simulation, this will record the final particle
  ///
  /// @param aTrack the current Geant4 track
  void PostUserTrackingAction(const G4Track* aTrack) final;

 protected:
  Config m_cfg;

 private:
  /// Convert a G4Track to a SimParticle
  ///
  /// @param aTrack the current Geant4 track
  SimParticle convert(const G4Track& aTrack) const;

  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
