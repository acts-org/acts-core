// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"

#include <memory>

#include <G4MagneticField.hh>

namespace Acts {
class MagneticFieldProvider;
}

namespace ActsExamples {

/// A magnetic field wrapper for the Acts magnetic field
/// to be used with Geant4.
class MagneticFieldWrapper : public G4MagneticField {
 public:
  /// Configuration of the Magnetic Field Action
  struct Config {
    /// Access to the ACTS magnetic field
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField = nullptr;
  };

  /// Construct the magnetic field action
  ///
  /// @param cfg the configuration struct
  /// @param logger the ACTS logging instance
  MagneticFieldWrapper(const Config& cfg,
                       std::unique_ptr<const Acts::Logger> logger =
                           Acts::getDefaultLogger("MagneticFieldWrapper",
                                                  Acts::Logging::INFO));
  ~MagneticFieldWrapper() override = default;

  /// Public get field interface
  ///
  /// @param Point is the field request point
  /// @param Bfield [in,out] is the field value
  ///
  void GetFieldValue(const G4double Point[4], G4double* Bfield) const final;

 protected:
  Config m_cfg;

 private:
  /// Private access method to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }

  /// The looging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace ActsExamples
