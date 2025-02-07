// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

namespace Acts {

/// @class InfiniteBounds
///
/// templated boundless extension to forward the interface
/// Returns all inside checks to true and can templated for all bounds
///
class InfiniteBounds : public SurfaceBounds {
 public:
  BoundsType type() const final { return SurfaceBounds::eBoundless; }

  std::vector<double> values() const final { return {}; }

  /// Method inside() returns true for any case
  ///
  /// ignores input parameters
  ///
  /// @return always true
  bool inside(const Vector2& /*lposition*/,
              const BoundaryTolerance& /*boundaryTolerance*/) const final {
    return true;
  }

  /// Output Method for std::ostream
  std::ostream& toStream(std::ostream& os) const final {
    os << "Acts::InfiniteBounds ... boundless surface" << std::endl;
    return os;
  }
};

static const InfiniteBounds s_noBounds{};

}  // namespace Acts
