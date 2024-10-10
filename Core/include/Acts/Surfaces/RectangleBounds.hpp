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
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <array>
#include <cassert>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <vector>

namespace Acts {

/// @class RectangleBounds
///
/// @image html RectangleBounds.gif
///
/// Bounds for a rectangular, planar surface - it can be used to for
/// rectangles that are symmetrically centered around (0./0.) and for
/// generic shifted rectangles
class RectangleBounds : public PlanarBounds {
 public:
  enum BoundValues : int {
    eMinX = 0,
    eMinY = 1,
    eMaxX = 2,
    eMaxY = 3,
    eSize = 4
  };

  RectangleBounds() = delete;

  /// Constructor with halflength in x and y - symmetric
  ///
  /// @param halfX halflength in X
  /// @param halfY halflength in Y
  RectangleBounds(double halfX, double halfY) noexcept(false)
      : m_min({-halfX, -halfY}), m_max({halfX, halfY}) {
    checkConsistency();
  }

  /// Constructor - from fixed size array - generic
  ///
  /// @param values The parameter values
  RectangleBounds(const std::array<double, eSize>& values) noexcept(false)
      : m_min({values[eMinX], values[eMinY]}),
        m_max({values[eMaxX], values[eMaxY]}) {
    checkConsistency();
  }

  /// Constructor - from min/max - generic
  ///
  /// @param min The left bottom corner
  /// @param max The right top corning
  RectangleBounds(const Vector2& min, const Vector2& max) noexcept(false)
      : m_min(min), m_max(max) {
    checkConsistency();
  }

  ~RectangleBounds() override = default;

  BoundsType type() const final;

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

  /// Return the vertices
  ///
  /// @param quarterSegments is the number of segments used to describe curved
  /// segments in a quarter of the phi range.
  /// @note the number of segments is ignored in this representation
  ///
  /// @return vector for vertices in 2D
  std::vector<Vector2> vertices(unsigned int quarterSegments = 0u) const final;

  // Bounding box representation
  const RectangleBounds& boundingBox() const final;

  /// Output Method for std::ostream
  ///
  /// @param sl is the ostream for the dump
  std::ostream& toStream(std::ostream& sl) const final;

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const;

  /// Access to the half length in X
  double halfLengthX() const;

  /// Access to the half length in Y
  double halfLengthY() const;

  /// Get the min vertex defining the bounds
  /// @return The min vertex
  const Vector2& min() const;

  /// Get the max vertex defining the bounds
  /// @return The max vertex
  const Vector2& max() const;

 private:
  Vector2 m_min;
  Vector2 m_max;

  /// Check the input values for consistency, will throw a logic_exception
  /// if consistency is not given
  void checkConsistency() noexcept(false);
};

inline SurfaceBounds::BoundsType RectangleBounds::type() const {
  return SurfaceBounds::eRectangle;
}

inline const Vector2& RectangleBounds::min() const {
  return m_min;
}

inline const Vector2& RectangleBounds::max() const {
  return m_max;
}

inline double RectangleBounds::halfLengthX() const {
  return 0.5 * (m_max.x() - m_min.x());
}

inline double RectangleBounds::halfLengthY() const {
  return 0.5 * (m_max.y() - m_min.y());
}

inline std::vector<double> RectangleBounds::values() const {
  return {m_min.x(), m_min.y(), m_max.x(), m_max.y()};
}

inline double RectangleBounds::get(BoundValues bValue) const {
  switch (bValue) {
    case eMinX:
      return m_min.x();
    case eMinY:
      return m_min.y();
    case eMaxX:
      return m_max.x();
    case eMaxY:
      return m_max.y();
    default:
      assert(false && "Invalid BoundValue enum value");
      return std::numeric_limits<double>::quiet_NaN();
  }
}

inline void RectangleBounds::checkConsistency() noexcept(false) {
  if (get(eMinX) > get(eMaxX)) {
    throw std::invalid_argument("RectangleBounds: invalid local x setup");
  }
  if (get(eMinY) > get(eMaxY)) {
    throw std::invalid_argument("RectangleBounds: invalid local y setup");
  }
}

}  // namespace Acts
