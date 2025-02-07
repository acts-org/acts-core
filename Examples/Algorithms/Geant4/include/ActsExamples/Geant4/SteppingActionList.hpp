// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <memory>
#include <vector>

#include <G4UserSteppingAction.hh>

namespace ActsExamples::Geant4 {

/// Geant4 only allows one user action of each type. This simple wrapper
/// dispatches multiple actions to Geant4.
class SteppingActionList : public G4UserSteppingAction {
 public:
  struct Config {
    std::vector<std::shared_ptr<G4UserSteppingAction>> actions;
  };

  explicit SteppingActionList(const Config &cfg) : m_cfg(cfg) {}

  void UserSteppingAction(const G4Step *step) override {
    for (const auto &action : m_cfg.actions) {
      if (action) {
        action->UserSteppingAction(step);
      }
    }
  }

 private:
  Config m_cfg;
};

}  // namespace ActsExamples::Geant4
