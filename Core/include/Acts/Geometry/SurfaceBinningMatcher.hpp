// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <utility>
#include <vector>

namespace Acts {

struct SurfaceBinningMatcher {
  /// The binning tolerance parameters
  using Range = std::pair<double, double>;
  std::vector<Range> tolerances{static_cast<int>(numAxisDirections()),
                                {0., 0.}};

  SurfaceBinningMatcher() = default;

  SurfaceBinningMatcher(const std::vector<Range>& tolpars)
      : tolerances(tolpars) {}

  /// Check function for surface equivalent
  ///
  /// @param gctx [in] gctx the geometry context for this check
  /// @param aDir the axis direction value for the binning
  /// @param one first surface for checking
  /// @param other second surface for checking
  bool operator()(const Acts::GeometryContext& gctx, Acts::AxisDirection aDir,
                  const Acts::Surface* one, const Acts::Surface* other) const {
    // Fast exit
    if (one == other) {
      return true;
    }

    auto oneExt = one->polyhedronRepresentation(gctx, 1).extent();
    auto otherExt = other->polyhedronRepresentation(gctx, 1).extent();

    double oneMin = oneExt.min(aDir);
    double oneMax = oneExt.max(aDir);

    double otherMin = otherExt.min(aDir);
    double otherMax = otherExt.max(aDir);

    return (
        std::abs(oneMin - otherMin) <= tolerances[toUnderlying(aDir)].first &&
        std::abs(oneMax - otherMax) <= tolerances[toUnderlying(aDir)].second);
  }
};

}  // namespace Acts
