// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <memory>

#include <G4VUserDetectorConstruction.hh>

namespace ActsExamples {

/// Silly Geant4 will destroy the detector construction after the run manager is
/// destructed. This class works around it by factorizing a factory.
class DetectorConstructionFactory {
 public:
  virtual ~DetectorConstructionFactory() = default;

  virtual std::unique_ptr<G4VUserDetectorConstruction> factorize() const = 0;
};

}  // namespace ActsExamples
