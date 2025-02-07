// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Geant4/EventStore.hpp"

#include <memory>
#include <string>

#include <G4UserSteppingAction.hh>

class G4VPhysicalVolume;
class G4Step;

namespace Acts {
class Surface;
}

namespace ActsExamples::Geant4 {

/// The G4SteppingAction that is called for every step in
/// the simulation process.
///
/// It checks whether a sensitive volume is present (via string tag)
/// and records (if necessary) the hit.
class SensitiveSteppingAction : public G4UserSteppingAction {
 public:
  /// Configuration of the Stepping action
  struct Config {
    std::shared_ptr<EventStore> eventStore;

    /// Selection for hit recording
    bool charged = true;
    bool neutral = false;
    bool primary = true;
    bool secondary = true;

    /// step logging mode
    bool stepLogging = false;
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
  void UserSteppingAction(const G4Step* step) override;

  /// Set the multimap that correlates G4VPhysicalVolumes to Acts::Surfaces
  ///
  /// @param surfaceMapping the multimap of physical volumes to surfaces
  void assignSurfaceMapping(
      const std::multimap<const G4VPhysicalVolume*, const Acts::Surface*>&
          surfaceMapping) {
    m_surfaceMapping = surfaceMapping;
  }

 protected:
  Config m_cfg;

 private:
  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// Private access method to the event store
  EventStore& eventStore() const { return *m_cfg.eventStore; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;

  std::multimap<const G4VPhysicalVolume*, const Acts::Surface*>
      m_surfaceMapping;
};

}  // namespace ActsExamples::Geant4
