// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

namespace Acts {

/// @ingroup MagneticField
/// @brief Null bfield which returns 0 always
class NullBField final : public MagneticFieldProvider {
 public:
  struct Cache {
    /// @brief constructor with context
    Cache(const MagneticFieldContext& /*mcfg*/) {}
  };

  /// @brief Default constructor
  NullBField() = default;

  /// @copydoc MagneticFieldProvider::getField(const Vector3&,MagneticFieldProvider::Cache&) const
  ///
  /// @note The @p position is ignored and only kept as argument to provide
  ///       a consistent interface with other magnetic field services.
  Result<Vector3> getField(const Vector3& position,
                           MagneticFieldProvider::Cache& cache) const override {
    (void)position;
    (void)cache;
    return Result<Vector3>::success(m_BField);
  }

  /// @copydoc MagneticFieldProvider::getFieldGradient(const Vector3&,ActsMatrix<3,3>&,MagneticFieldProvider::Cache&) const
  ///
  /// @note The @p position is ignored and only kept as argument to provide
  ///       a consistent interface with other magnetic field services.
  /// @note currently the derivative is not calculated
  /// @todo return derivative
  Result<Vector3> getFieldGradient(
      const Vector3& position, ActsMatrix<3, 3>& derivative,
      MagneticFieldProvider::Cache& cache) const override {
    (void)position;
    (void)derivative;
    (void)cache;
    return Result<Vector3>::success(m_BField);
  }

  /// @copydoc MagneticFieldProvider::makeCache(const MagneticFieldContext&) const
  Acts::MagneticFieldProvider::Cache makeCache(
      const Acts::MagneticFieldContext& mctx) const override {
    return Acts::MagneticFieldProvider::Cache(std::in_place_type<Cache>, mctx);
  }

  /// @brief check whether given 3D position is inside look-up domain
  ///
  /// @return @c true if position is inside the defined look-up grid,
  ///         otherwise @c false
  /// @note The method will always return true for the null B-Field
  bool isInside(const Vector3& /*position*/) const { return true; }

 private:
  /// magnetic field vector
  const Vector3 m_BField = Vector3::Zero();
};
}  // namespace Acts
