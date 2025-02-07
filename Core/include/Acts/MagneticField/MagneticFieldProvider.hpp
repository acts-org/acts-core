// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Utilities/Any.hpp"
#include "Acts/Utilities/Result.hpp"

namespace Acts {

/// @defgroup MagneticField Magnetic field

/// Base class for all magnetic field providers
class MagneticFieldProvider {
 public:
  /// Opaque cache type that can store arbitrary implementation specific cache
  /// data. Examples are an interpolation cell, or an experiment specific
  /// conditions data handle.
  using Cache = Acts::AnyBase<sizeof(char) * 512>;

  /// Make an opaque cache for the magnetic field. Instructs the specific
  /// implementation to generate a @c Cache instance for magnetic field lookup.
  ///
  /// @param mctx The magnetic field context to generate cache for
  /// @return Cache The opaque cache object
  virtual Cache makeCache(const MagneticFieldContext& mctx) const = 0;

  /// Retrieve magnetic field value at a given location. Requires a cache object
  /// created through makeCache().
  ///
  /// @param [in] position global 3D position for the lookup
  /// @param [in,out] cache Field provider specific cache object
  ///
  /// @return magnetic field vector at given position
  virtual Result<Vector3> getField(const Vector3& position,
                                   Cache& cache) const = 0;

  virtual ~MagneticFieldProvider() = default;
};

}  // namespace Acts
