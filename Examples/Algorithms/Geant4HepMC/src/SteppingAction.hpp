// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <vector>

#include <G4UserSteppingAction.hh>
#include <HepMC3/GenParticle.h>
#include <HepMC3/GenVertex.h>
#include <globals.hh>

namespace ActsExamples::Geant4::HepMC3 {

/// Collects the particles history.
class SteppingAction : public G4UserSteppingAction {
 public:
  explicit SteppingAction(std::vector<std::string> eventRejectionProcess);
  ~SteppingAction() override;

  /// Static access method to the instance
  static SteppingAction* instance();

  /// @brief Interface Method doing the step and records the data
  /// @param step is the Geant4 step of the particle
  void UserSteppingAction(const G4Step* step) override;

  /// Interface reset method
  void clear();

  /// Return the abort status
  bool eventAborted() { return m_eventAborted; }

 private:
  /// Instance of the SteppingAction
  static SteppingAction* s_instance;
  /// The end vertex of the previous step
  std::shared_ptr<::HepMC3::GenVertex> m_previousVertex = nullptr;
  /// List to veto events with certain processes
  std::vector<std::string> m_eventRejectionProcess;
  /// States whether an event was aborted
  bool m_eventAborted = false;
};
}  // namespace ActsExamples::Geant4::HepMC3
