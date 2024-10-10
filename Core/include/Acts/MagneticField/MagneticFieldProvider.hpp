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
#include "Acts/Utilities/Any.hpp"
#include "Acts/Utilities/Result.hpp"

#include <array>
#include <memory>

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

  /// Retrieve magnetic field value its its gradient. Requires a cache object
  /// created through makeCache().
  ///
  /// @param [in]  position   global 3D position
  /// @param [out] derivative gradient of magnetic field vector as (3x3) matrix
  /// @param [in,out] cache Field provider specific cache object
  /// @return magnetic field vector
  virtual Result<Vector3> getFieldGradient(const Vector3& position,
                                           ActsMatrix<3, 3>& derivative,
                                           Cache& cache) const = 0;

  virtual ~MagneticFieldProvider();
};

inline MagneticFieldProvider::~MagneticFieldProvider() = default;

}  // namespace Acts
