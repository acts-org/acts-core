// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <functional>
#include <memory>

class G4VUserPhysicsList;

namespace ActsExamples {

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

  PhysicsListFactoryFunction(Function function);

  std::unique_ptr<G4VUserPhysicsList> factorize() const final;

 private:
  Function m_function;
};

}  // namespace ActsExamples
