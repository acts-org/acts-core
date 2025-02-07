// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geant4/PhysicsListFactory.hpp"

#include <G4VUserPhysicsList.hh>

namespace ActsExamples::Geant4 {

PhysicsListFactoryFunction::PhysicsListFactoryFunction(Function function)
    : m_function(std::move(function)) {}

std::unique_ptr<G4VUserPhysicsList> PhysicsListFactoryFunction::factorize()
    const {
  return m_function();
}

}  // namespace ActsExamples::Geant4
