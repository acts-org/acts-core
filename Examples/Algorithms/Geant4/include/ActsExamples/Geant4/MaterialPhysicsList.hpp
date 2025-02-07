// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>

#include <G4VUserPhysicsList.hh>

namespace ActsExamples::Geant4 {

/// @class MaterialPhysicsList
///
/// @brief Stripped down physics list to Geantinos and only Transport
/// This speeds up the initialization of the MaterialRecording job
///
class MaterialPhysicsList final : public G4VUserPhysicsList {
 public:
  /// Construct the action
  ///
  /// @param cfg the configuration struct for this Stepping action
  /// @param logger is an Acts::Logger for unique logging
  explicit MaterialPhysicsList(std::unique_ptr<const Acts::Logger> logger =
                                   Acts::getDefaultLogger("MaterialPhysicsList",
                                                          Acts::Logging::INFO));
  ~MaterialPhysicsList() override = default;

  /// @brief Interface particle construction method
  ///
  /// This will add only Geantino and ChargedGeantino
  void ConstructParticle() override;

  /// @brief Interface process construction method
  ///
  /// This will add only Transport
  void ConstructProcess() override;

  /// @brief Interface process construction method
  ///
  /// This will add only Transport
  void SetCuts() override;

 private:
  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples::Geant4
