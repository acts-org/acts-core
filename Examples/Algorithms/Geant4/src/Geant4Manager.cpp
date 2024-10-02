// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/Geant4Manager.hpp"

#include "ActsExamples/Geant4/MaterialPhysicsList.hpp"
#include "ActsExamples/Geant4/PhysicsListFactory.hpp"

#include <memory>
#include <stdexcept>

#include <FTFP_BERT.hh>
#include <FTFP_BERT_ATL.hh>
#include <G4EmParameters.hh>
#include <G4HadronicParameters.hh>
#include <G4HadronicProcessStore.hh>
#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4UserEventAction.hh>
#include <G4UserRunAction.hh>
#include <G4UserSteppingAction.hh>
#include <G4UserTrackingAction.hh>
#include <G4VUserDetectorConstruction.hh>
#include <G4VUserPhysicsList.hh>
#include <G4VUserPrimaryGeneratorAction.hh>
#include <G4Version.hh>

namespace ActsExamples {

Geant4Handle::Geant4Handle(std::unique_ptr<G4RunManager> _runManager,
                           std::unique_ptr<G4VUserPhysicsList> _physicsList,
                           std::string _physicsListName)
    : runManager(std::move(_runManager)),
      physicsList(_physicsList.release()),
      physicsListName(std::move(_physicsListName)) {
  if (runManager == nullptr) {
    std::invalid_argument("runManager cannot be null");
  }
  if (physicsList == nullptr) {
    std::invalid_argument("physicsList cannot be null");
  }

  // Set physics list
  runManager->SetUserInitialization(physicsList);
}

Geant4Handle::~Geant4Handle() = default;

void Geant4Handle::tweakLogging(int level) const {
  Geant4Manager::tweakLogging(*runManager, level);
}

Geant4Manager& Geant4Manager::instance() {
  static Geant4Manager manager;
  return manager;
}

void Geant4Manager::tweakLogging(G4RunManager& runManager, int level) {
  runManager.SetVerboseLevel(level);
  G4EventManager::GetEventManager()->SetVerboseLevel(level);
  G4EventManager::GetEventManager()->GetTrackingManager()->SetVerboseLevel(
      level);
  G4EventManager::GetEventManager()->GetStackManager()->SetVerboseLevel(level);

  // Suppress the printing of physics information.
#if G4VERSION_NUMBER >= 1100
  G4HadronicParameters::Instance()->SetVerboseLevel(0);
  G4HadronicProcessStore::Instance()->SetVerbose(0);
  G4EmParameters::Instance()->SetIsPrintedFlag(true);
#endif
}

std::shared_ptr<Geant4Handle> Geant4Manager::currentHandle() const {
  return m_handle.lock();
}

std::shared_ptr<Geant4Handle> Geant4Manager::createHandle(
    const std::string& physicsList) {
  return createHandle(createPhysicsList(physicsList), physicsList);
}

std::shared_ptr<Geant4Handle> Geant4Manager::createHandle(
    std::unique_ptr<G4VUserPhysicsList> physicsList,
    std::string physicsListName) {
  if (!m_handle.expired()) {
    throw std::runtime_error("creating a second handle is prohibited");
  }
  if (m_created) {
    throw std::runtime_error(
        "creating a new handle is prohibited. you have to hold onto the "
        "first one.");
  }

  auto runManager = std::unique_ptr<G4RunManager>(
      G4RunManagerFactory::CreateRunManager(G4RunManagerType::SerialOnly));

  auto handle = std::make_shared<Geant4Handle>(std::move(runManager),
                                               std::move(physicsList),
                                               std::move(physicsListName));

  m_created = true;
  m_handle = handle;
  return handle;
}

void Geant4Manager::registerPhysicsListFactory(
    std::string name, std::shared_ptr<PhysicsListFactory> physicsListFactory) {
  if (m_physicsListFactories.contains(name)) {
    throw std::invalid_argument("name already mapped");
  }
  m_physicsListFactories.emplace(std::move(name),
                                 std::move(physicsListFactory));
}

std::unique_ptr<G4VUserPhysicsList> Geant4Manager::createPhysicsList(
    const std::string& name) const {
  auto it = m_physicsListFactories.find(name);
  if (it == m_physicsListFactories.end()) {
    throw std::invalid_argument("name not mapped");
  }
  return it->second->factorize();
}

const std::unordered_map<std::string, std::shared_ptr<PhysicsListFactory>>&
Geant4Manager::getPhysicsListFactories() const {
  return m_physicsListFactories;
}

Geant4Manager::Geant4Manager() {
  registerPhysicsListFactory(
      "FTFP_BERT", std::make_shared<PhysicsListFactoryFunction>(
                       []() { return std::make_unique<FTFP_BERT>(); }));
  registerPhysicsListFactory(
      "FTFP_BERT_ATL", std::make_shared<PhysicsListFactoryFunction>(
                           []() { return std::make_unique<FTFP_BERT_ATL>(); }));
  registerPhysicsListFactory("MaterialPhysicsList",
                             std::make_shared<PhysicsListFactoryFunction>([]() {
                               return std::make_unique<MaterialPhysicsList>();
                             }));
}

Geant4Manager::~Geant4Manager() = default;

}  // namespace ActsExamples
