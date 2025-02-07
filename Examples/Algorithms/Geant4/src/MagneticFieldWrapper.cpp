// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

  auto fieldRes = m_cfg.magneticField->getField(
      {convertLength * Point[0], convertLength * Point[1],
       convertLength * Point[2]},
      bCache);
  if (!fieldRes.ok()) {
    ACTS_ERROR("Field lookup error: " << fieldRes.error());
    return;
  }
  // Get the field now
  const Acts::Vector3& field = *fieldRes;

  Bfield[0] = convertField * field[0];
  Bfield[1] = convertField * field[1];
  Bfield[2] = convertField * field[2];
}

}  // namespace ActsExamples::Geant4
