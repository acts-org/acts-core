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

namespace ActsExamples {

/// The ScalableBField-specific magnetic field context.
struct ScalableBFieldContext {
  double scalor = 1.;
};

/// A constant magnetic field that is scaled depending on the event context.
class ScalableBField final : public Acts::MagneticFieldProvider {
 public:
  struct Cache {
    double scalor = 1.;

    /// @brief constructor with context
    explicit Cache(const Acts::MagneticFieldContext& mctx) {
      scalor = mctx.get<const ScalableBFieldContext>().scalor;
    }
  };

  /// @brief construct constant magnetic field from field vector
  ///
  /// @param [in] B magnetic field vector in global coordinate system
  explicit ScalableBField(Acts::Vector3 B) : m_BField(std::move(B)) {}

  /// @brief construct constant magnetic field from components
  ///
  /// @param [in] Bx magnetic field component in global x-direction
  /// @param [in] By magnetic field component in global y-direction
  /// @param [in] Bz magnetic field component in global z-direction
  explicit ScalableBField(double Bx = 0, double By = 0, double Bz = 0)
      : m_BField(Bx, By, Bz) {}

  /// @brief retrieve magnetic field value
  ///
  /// @param [in] position global position
  /// @param [in] cache Cache object (is ignored)
  /// @return magnetic field vector
  ///
  /// @note The @p position is ignored and only kept as argument to provide
  ///       a consistent interface with other magnetic field services.
  Acts::Result<Acts::Vector3> getField(
      const Acts::Vector3& /*position*/,
      MagneticFieldProvider::Cache& gCache) const override {
    Cache& cache = gCache.as<Cache>();
    return Acts::Result<Acts::Vector3>::success(m_BField * cache.scalor);
  }

  Acts::MagneticFieldProvider::Cache makeCache(
      const Acts::MagneticFieldContext& mctx) const override {
    return Acts::MagneticFieldProvider::Cache(std::in_place_type<Cache>, mctx);
  }

  /// @brief check whether given 3D position is inside look-up domain
  ///
  /// @param [in] position global 3D position
  /// @return @c true if position is inside the defined look-up grid,
  ///         otherwise @c false
  /// @note The method will always return true for the constant B-Field
  bool isInside(const Acts::Vector3& /*position*/) const { return true; }

  /// @brief update magnetic field vector from components
  ///
  /// @param [in] Bx magnetic field component in global x-direction
  /// @param [in] By magnetic field component in global y-direction
  /// @param [in] Bz magnetic field component in global z-direction
  void setField(double Bx, double By, double Bz) { m_BField << Bx, By, Bz; }

  /// @brief update magnetic field vector
  ///
  /// @param [in] B magnetic field vector in global coordinate system
  void setField(const Acts::Vector3& B) { m_BField = B; }

 private:
  /// magnetic field vector
  Acts::Vector3 m_BField;
};

}  // namespace ActsExamples
