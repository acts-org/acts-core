// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4/MagneticFieldWrapper.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

#include <utility>

#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>

namespace ActsExamples::Geant4 {

MagneticFieldWrapper::MagneticFieldWrapper(
    const Config& cfg, std::unique_ptr<const Acts::Logger> logger)
    : G4MagneticField(), m_cfg(cfg), m_logger(std::move(logger)) {}

void MagneticFieldWrapper::GetFieldValue(const G4double Point[4],
                                         G4double* Bfield) const {
  constexpr double convertLength = CLHEP::mm / Acts::UnitConstants::mm;
  constexpr double convertField = CLHEP::tesla / Acts::UnitConstants::T;

  auto bCache = m_cfg.magneticField->makeCache(Acts::MagneticFieldContext());

  auto field = m_cfg.magneticField->getField(
      {convertLength * Point[0], convertLength * Point[1],
       convertLength * Point[2]},
      bCache);

  Bfield[0] = convertField * field[0];
  Bfield[1] = convertField * field[1];
  Bfield[2] = convertField * field[2];
}

}  // namespace ActsExamples::Geant4
