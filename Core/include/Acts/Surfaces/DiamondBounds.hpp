// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iosfwd>
#include <stdexcept>
#include <vector>

namespace Acts {

/// @class DiamondBounds
///
/// @image html DiamondBounds.svg
/// Bounds for a double trapezoidal ("diamond"), planar Surface.
class DiamondBounds : public PlanarBounds {
 public:
  enum BoundValues {
    eHalfLengthXnegY = 0,
    eHalfLengthXzeroY = 1,
    eHalfLengthXposY = 2,
    eHalfLengthYneg = 3,
    eHalfLengthYpos = 4,
    eSize = 5
  };

  DiamondBounds() = delete;

  /// Constructor for convex hexagon symmetric about the y axis
  ///
  /// @param halfXnegY is the halflength in x at minimal y
  /// @param halfXzeroY is the halflength in x at y = 0
  /// @param halfXposY is the halflength in x at maximal y
  /// @param halfYneg is the halflength into y < 0
  /// @param halfYpos is the halflength into y > 0
  DiamondBounds(double halfXnegY, double halfXzeroY, double halfXposY,
                double halfYneg, double halfYpos) noexcept(false)
      : m_values({halfXnegY, halfXzeroY, halfXposY, halfYneg, halfYpos}),
        m_boundingBox(
            Vector2{
                -(*std::max_element(m_values.begin(), m_values.begin() + 2)),
                -halfYneg},
            Vector2{*std::max_element(m_values.begin(), m_values.begin() + 2),
                    halfYpos}) {
    checkConsistency();
  }

  /// Constructor - from fixed size array
  ///
  /// @param values The parameter values
  DiamondBounds(const std::array<double, eSize>& values) noexcept(false)
      : m_values(values),
        m_boundingBox(
            Vector2{-(*std::max_element(values.begin(), values.begin() + 2)),
                    -values[eHalfLengthYneg]},
            Vector2{*std::max_element(values.begin(), values.begin() + 2),
                    values[eHalfLengthYpos]}) {}

  ~DiamondBounds() override = default;

  BoundsType type() const final;

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
  /// @return boolean indicator for the success of this operation
  bool inside(const Vector2& lposition,
              const BoundaryTolerance& boundaryTolerance) const final;

  /// Return the vertices that describe this shape
  ///
  /// @param ignoredSegments is an ignored parameter only used for
  /// curved bound segments
  ///
  /// @return vector for vertices in 2D
  std::vector<Vector2> vertices(unsigned int ignoredSegments = 0u) const final;

  // Bounding box representation
  const RectangleBounds& boundingBox() const final;

  /// Output Method for std::ostream
  ///
  /// @param sl is the ostream in which it is dumped
  std::ostream& toStream(std::ostream& sl) const final;

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const { return m_values[bValue]; }

 private:
  std::array<double, eSize> m_values;
  RectangleBounds m_boundingBox;  ///< internal bounding box cache

  /// Check the input values for consistency, will throw a logic_exception
  /// if consistency is not given
  void checkConsistency() noexcept(false);
};

inline std::vector<double> DiamondBounds::values() const {
  std::vector<double> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

inline void DiamondBounds::checkConsistency() noexcept(false) {
  if (std::ranges::any_of(m_values, [](auto v) { return v <= 0.; })) {
    throw std::invalid_argument("DiamondBounds: negative half length.");
  }
  if (get(eHalfLengthXnegY) > get(eHalfLengthXzeroY) ||
      get(eHalfLengthXposY) > get(eHalfLengthXzeroY)) {
    throw std::invalid_argument("DiamondBounds: not a diamond shape.");
  }
}

}  // namespace Acts
