// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/DD4hep/DD4hepFieldAdapter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

#include <DD4hep/Fields.h>
#include <DD4hep/Handle.h>
#include <DD4hep/Objects.h>

namespace Acts {

DD4hepFieldAdapter::DD4hepFieldAdapter(dd4hep::OverlayedField field)
    : m_field{std::make_unique<dd4hep::OverlayedField>(field)} {
  m_fieldConversionFactor =
      dd4hep::_toDouble("1/tesla") * Acts::UnitConstants::T;
  m_lengthConversionFactor =
      dd4hep::_toDouble("1*mm") / Acts::UnitConstants::mm;
}

MagneticFieldProvider::Cache DD4hepFieldAdapter::makeCache(
    const Acts::MagneticFieldContext& /*mctx*/) const {
  return MagneticFieldProvider::Cache{};
}

Vector3 DD4hepFieldAdapter::getField(
    const Vector3& position, MagneticFieldProvider::Cache& /*cache*/) const {
  dd4hep::Position dd4hepPosition{position.x(), position.y(), position.z()};

  // ACTS mm -> dd4hep mm
  dd4hepPosition *= m_lengthConversionFactor;

  const auto direction = m_field->combinedMagnetic(dd4hepPosition);

  Vector3 result{direction.x(), direction.y(), direction.z()};

  // dd4hep tesla -> ACTS tesla
  result *= m_fieldConversionFactor;

  return result;
}

std::pair<Vector3, SquareMatrix3> DD4hepFieldAdapter::getFieldAndGradient(
    const Vector3& /*position*/,
    MagneticFieldProvider::Cache& /*cache*/) const {
  throw std::runtime_error("not implemented");
}

}  // namespace Acts
