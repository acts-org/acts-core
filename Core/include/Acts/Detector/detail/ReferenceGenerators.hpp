// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <vector>

namespace Acts::Experimental::detail {

/// A struct to access the center position
///
/// This generator will provide only one filling point and hence
/// only a single bin in the indexed grid.
struct CenterReferenceGenerator {
  /// Helper to access the Center point of for filling the grid
  ///
  /// @param gctx the geometry context of this operation
  /// @param surface the surface for which the reference point is to be accessed
  ///
  /// @return a vector of reference points for filling
  const std::vector<Vector3> references(const GeometryContext& gctx,
                                        const Surface& surface) const {
    return {surface.center(gctx)};
  }
};

/// A struct to access reference positions based on bin values
///
/// @tparam bVAL the binning value to be used for the binning position call
///
/// This generator will provide only one filling point and hence
/// only a single bin in the indexed grid.
template <BinningValue bVAL>
struct BinningValueReferenceGenerator {
  /// Helper to access a reference position based on binning value
  ///
  /// @param gctx the geometry context of this operation
  /// @param surface the surface for which the reference point is to be accessed
  ///
  /// @return a vector of reference points for filling
  const std::vector<Vector3> references(const GeometryContext& gctx,
                                        const Surface& surface) const {
    return {surface.binningPosition(gctx, bVAL)};
  }
};

/// A struct to access generated vertices from surface polyhedrons
/// These vertices are then used to find the bin boundary box for the
/// indexed grid.
///
/// @tparam nSEGS the number of segments to be used for the polyhedron
/// approximation of arcs between vertices
/// @tparam aBARY if true, the barycenter of the polyhedron is added
///
/// The grid filling then completes the empty bins in between and
/// expands if necessary.
template <std::size_t nSEGS = 1u, bool aBARY = true>
struct PolyhedronReferenceGenerator {
  /// Helper to access the Center point of for filling the grid
  ///
  /// @param gctx the geometry context of this operation
  /// @param surface the surface for which the reference point is to be accessed
  ///
  /// @return a vector of reference points for filling
  const std::vector<Vector3> references(const GeometryContext& gctx,
                                        const Surface& surface) const {
    // Create the return  vector
    std::vector<Vector3> rPositions;
    auto pHedron = surface.polyhedronRepresentation(gctx, nSEGS);
    rPositions.insert(rPositions.end(), pHedron.vertices.begin(),
                      pHedron.vertices.end());
    // Add the barycenter if configured
    if constexpr (aBARY) {
      Vector3 bc(0., 0., 0.);
      std::for_each(rPositions.begin(), rPositions.end(),
                    [&](const auto& p) { bc += p; });
      bc *= 1. / rPositions.size();
      rPositions.push_back(bc);
    }
    return rPositions;
  }
};

}  // namespace Acts::Experimental::detail
