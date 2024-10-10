// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

namespace Acts {

/// @class InfiniteBounds
///
/// templated boundless extension to forward the interface
/// Returns all inside checks to true and can templated for all bounds

class InfiniteBounds : public SurfaceBounds {
 public:
  InfiniteBounds() = default;

  ~InfiniteBounds() override = default;

  SurfaceBounds::BoundsType type() const final {
    return SurfaceBounds::eBoundless;
  }

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
