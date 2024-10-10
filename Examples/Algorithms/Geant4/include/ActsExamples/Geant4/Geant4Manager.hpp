// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class G4RunManager;
class G4VUserPhysicsList;

namespace ActsExamples {

class PhysicsListFactory;
class Geant4Manager;

/// Manages the life time of G4RunManager and G4VUserPhysicsList.
///
/// G4RunManager must only be instantiated once and must be deleted at a later
/// point before main ends.
///
/// In principle it should be possible to use multiple G4VUserPhysicsList
/// objects during one G4RunManager lifecycle. But Geant4 does not provide a
/// single way to achieve this and the user has to modify the correct state
/// variables themselves. In this case we would bind much thighter on the API
/// and future changes could potentially break our Geant4 state assumption. This
/// is why the current interface only allows for a single G4VUserPhysicsList.
///
/// TODO A way out of this Geant4 lifecycle mess might be dynamically unloading
/// and loading the Geant4 library which should reset it to its original state.
struct Geant4Handle {
  std::mutex mutex;
  std::unique_ptr<G4RunManager> runManager;
  G4VUserPhysicsList *physicsList;
  std::string physicsListName;

  Geant4Handle(std::unique_ptr<G4RunManager> runManager,
               std::unique_ptr<G4VUserPhysicsList> physicsList,
               std::string physicsListName);
  Geant4Handle(const Geant4Handle &) = delete;
  Geant4Handle &operator=(const Geant4Handle &) = delete;
  ~Geant4Handle();

  /// Set logging consistently across common Geant4 modules
  ///
  /// Convenience method which calls into Geant4Manager
  void tweakLogging(int level) const;
};

/// Allows easy instantiation of a Geant4Handle object
class Geant4Manager {
 public:
  static Geant4Manager &instance();

  /// Set logging consistently across common Geant4 modules
  static void tweakLogging(G4RunManager &runManager, int level);

  std::shared_ptr<Geant4Handle> currentHandle() const;

  /// This can only be called once due to Geant4 limitations
  std::shared_ptr<Geant4Handle> createHandle(const std::string &physicsList);

  /// This can only be called once due to Geant4 limitations
  std::shared_ptr<Geant4Handle> createHandle(
      std::unique_ptr<G4VUserPhysicsList> physicsList,
      std::string physicsListName);

  /// Registers a named physics list factory to the manager for easy
  /// instantiation when needed.
  void registerPhysicsListFactory(
      std::string name, std::shared_ptr<PhysicsListFactory> physicsListFactory);
  std::unique_ptr<G4VUserPhysicsList> createPhysicsList(
      const std::string &name) const;

  /// Get the current list of physics list factories.
  const std::unordered_map<std::string, std::shared_ptr<PhysicsListFactory>> &
  getPhysicsListFactories() const;

 private:
  Geant4Manager();
  ~Geant4Manager();

  bool m_created = false;
  std::weak_ptr<Geant4Handle> m_handle;
  std::unordered_map<std::string, std::shared_ptr<PhysicsListFactory>>
      m_physicsListFactories;
};

}  // namespace ActsExamples
