// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// Workaround for building on clang+libstdc++
#include "Acts/Utilities/detail/ReferenceWrapperAnyCompat.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Vertexing/Vertex.hpp"

namespace Acts {

/// @brief Vertex Finder Options
///
struct VertexingOptions {
  /// Default constructor is deleted
  VertexingOptions() = delete;

  /// VertexingOptions with context and vertex constraint
  ///
  /// @param gctx Geometry context
  /// @param mctx Magnetic field context
  /// @param constr Vertex constraint
  /// @param useConstr Boolean indicating whether vertex constraint should be used during the vertex fit
  VertexingOptions(const GeometryContext& gctx,
                   const MagneticFieldContext& mctx, const Vertex& constr,
                   const bool useConstr = true)
      : geoContext(gctx),
        magFieldContext(mctx),
        constraint(constr),
        useConstraintInFit(useConstr) {
    if (useConstraintInFit && constraint.covariance().determinant() == 0.) {
      throw std::invalid_argument(
          "Vertex constraint covariance matrix must be invertible.");
    }
  }

  /// VertexingOptions with context and without vertex constraint
  ///
  /// @param gctx Geometry context
  /// @param mctx Magnetic field context
  VertexingOptions(const GeometryContext& gctx,
                   const MagneticFieldContext& mctx)
      : geoContext(gctx), magFieldContext(mctx) {
    constraint = Vertex();
    useConstraintInFit = false;
  }

  /// Context object for the geometry
  std::reference_wrapper<const GeometryContext> geoContext;
  /// Context object for the magnetic field
  std::reference_wrapper<const MagneticFieldContext> magFieldContext;
  /// Vertex constraint. Important note: While this variable is not used during
  /// the vertex fit if useConstraintInFit is set to false, it is always used
  /// during vertex finding.
  Vertex constraint;
  /// Boolean indicating whether we use the constraint information during
  /// the vertex fit. If set to true, the covariance matrix of constraint must
  /// be invertible.
  bool useConstraintInFit;
};

}  // namespace Acts
