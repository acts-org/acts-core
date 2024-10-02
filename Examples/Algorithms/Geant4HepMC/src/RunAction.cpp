// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
