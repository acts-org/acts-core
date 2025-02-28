// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <array>
#include <iosfwd>
#include <vector>

namespace Acts {

/// @class LineBounds
///
/// Bounds for a LineSurface.
class LineBounds : public SurfaceBounds {
 public:
  enum BoundValues : int { eR = 0, eHalfLengthZ = 1, eSize = 2 };

  /// Constructor
  ///
  /// @param r The radius of the line
  /// @param halfZ The half length in z
  explicit LineBounds(double r, double halfZ) noexcept(false)
      : m_values({r, halfZ}) {
    checkConsistency();
  }

  /// Constructor - from fixed size array
  ///
  /// @param values The bound values stored in a fixed size array
  explicit LineBounds(const std::array<double, eSize>& values) noexcept(false)
      : m_values(values) {
    checkConsistency();
  }

  BoundsType type() const final { return SurfaceBounds::eLine; }

  /// Return the bound values as dynamically sized vector
  ///
  /// @return this returns a copy of the internal values
  std::vector<double> values() const final;

  /// Inside check for the bounds object driven by the boundary check directive
  /// Each Bounds has a method inside, which checks if a LocalPosition is inside
  /// the bounds  Inside can be called without/with tolerances.
  ///
  /// @param lposition Local position (assumed to be in right surface frame)
  /// @param boundaryTolerance boundary check directive
  ///
  /// @return boolean indicator for the success of this operation
  bool inside(const Vector2& lposition,
              const BoundaryTolerance& boundaryTolerance) const final;

  /// Output Method for std::ostream
  ///
  /// @param sl is the ostream to be dumped into
  std::ostream& toStream(std::ostream& sl) const final;

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const { return m_values[bValue]; }

 private:
  std::array<double, eSize> m_values;

  /// Check the input values for consistency, will throw a logic_exception
  /// if consistency is not given
  void checkConsistency() noexcept(false);
};

}  // namespace Acts
