// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "RunAction.hpp"

#include <stdexcept>

#include <G4Run.hh>

#include "EventAction.hpp"

namespace ActsExamples::Geant4::HepMC3 {

RunAction* RunAction::s_instance = nullptr;

RunAction* RunAction::instance() {
  return s_instance;
}

RunAction::RunAction() : G4UserRunAction() {
  if (s_instance != nullptr) {
    throw std::logic_error("Attempted to duplicate the RunAction singleton");
  } else {
    s_instance = this;
  }
}

RunAction::~RunAction() {
  s_instance = nullptr;
}

void RunAction::BeginOfRunAction(const G4Run* /*run*/) {
  // initialize event cumulative quantities
  EventAction::instance()->clear();
}

void RunAction::EndOfRunAction(const G4Run* /*run*/) {}

}  // namespace ActsExamples::Geant4::HepMC3
