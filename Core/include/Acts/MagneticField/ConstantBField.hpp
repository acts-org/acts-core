// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

namespace Acts {

/// @ingroup MagneticField
///
/// This class implements a simple constant magnetic field. The
/// magnetic field value has to be set at creation time, but can
/// be updated later on.
class ConstantBField final : public MagneticFieldProvider {
 public:
  struct Cache {
    /// @brief constructor with context
    Cache(const MagneticFieldContext& /*mcfg*/) {}
  };

  /// Construct constant magnetic field from field vector.
  ///
  /// @param [in] B magnetic field vector in global coordinate system
  explicit ConstantBField(Vector3 B) : m_BField(std::move(B)) {}

  /// @brief Get the B field at a position
  Vector3 getField() const { return m_BField; }

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

  /// @copydoc MagneticFieldProvider::makeCache(const MagneticFieldContext&) const
  Acts::MagneticFieldProvider::Cache makeCache(
      const Acts::MagneticFieldContext& mctx) const override {
    return Acts::MagneticFieldProvider::Cache(std::in_place_type<Cache>, mctx);
  }

  /// @brief check whether given 3D position is inside look-up domain
  ///
  /// @return Always true for constant magnetic field
  bool isInside(const Vector3& /*position*/) const { return true; }

  /// @brief update magnetic field vector
  ///
  /// @param [in] B magnetic field vector in global coordinate system
  void setField(const Vector3& B) { m_BField = B; }

 private:
  /// magnetic field vector
  Vector3 m_BField;
};
}  // namespace Acts
