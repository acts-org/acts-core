// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/Volume.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Geant4/EventStore.hpp"

#include <memory>
#include <string>

#include <G4UserSteppingAction.hh>

class G4Step;
namespace Acts {
class Volume;
}  // namespace Acts

namespace ActsExamples::Geant4 {

/// A G4SteppingAction that is called for every step in the simulation process.
///
/// It checks whether the particle can be killed according to the user settings
/// e.g. if its position exceeds the configured values for |z| or r.
class ParticleKillAction : public G4UserSteppingAction {
 public:
  /// Configuration of the Stepping action
  struct Config {
    /// event store
    std::shared_ptr<EventStore> eventStore;

    /// particles outside this volume will be terminated
    std::shared_ptr<const Acts::Volume> volume;
    /// particles that exceed this global time limit will be terminated
    double maxTime = std::numeric_limits<double>::infinity();
    /// secondary particles will be terminated
    bool secondaries = false;
  };

  /// Construct the stepping action
  ///
  /// @param cfg the configuration struct
  /// @param logger the ACTS logging instance
  ParticleKillAction(const Config& cfg,
                     std::unique_ptr<const Acts::Logger> logger =
                         Acts::getDefaultLogger("ParticleKillAction",
                                                Acts::Logging::INFO));
  ~ParticleKillAction() override = default;

  /// @brief Called every step, conditionally sets the tracking state to `fStopAndKill`
  /// @param step is the Geant4 step of the particle
  void UserSteppingAction(const G4Step* step) override;

 private:
  /// Private access method to the event store
  EventStore& eventStore() const { return *m_cfg.eventStore; }

  const Acts::Logger& logger() const { return *m_logger; }

  Config m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples::Geant4
