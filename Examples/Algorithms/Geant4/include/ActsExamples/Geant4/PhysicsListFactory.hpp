// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <functional>
#include <memory>

class G4VUserPhysicsList;

namespace ActsExamples::Geant4 {

/// A factory around G4VUserPhysicsList which allows on demand instantiation.
class PhysicsListFactory {
 public:
  virtual ~PhysicsListFactory() = default;

  virtual std::unique_ptr<G4VUserPhysicsList> factorize() const = 0;
};

/// Convenience implementation of PhysicsListFactory from std::function
class PhysicsListFactoryFunction final : public PhysicsListFactory {
 public:
  using Function = std::function<std::unique_ptr<G4VUserPhysicsList>()>;

  explicit PhysicsListFactoryFunction(Function function);

  std::unique_ptr<G4VUserPhysicsList> factorize() const final;

 private:
  Function m_function;
};

}  // namespace ActsExamples::Geant4
