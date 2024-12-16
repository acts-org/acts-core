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
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <array>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iosfwd>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace Acts {

/// @class EllipseBounds
///
/// @image html EllipseBounds.png
///
/// Class to describe the bounds for a planar ellispoid
/// surface.
///
/// By providing an argument for hphisec, the bounds can
/// be restricted to a phi-range around the center position.
class EllipseBounds : public PlanarBounds {
 public:
  enum BoundValues {
    eInnerRx = 0,
    eInnerRy = 1,
    eOuterRx = 2,
    eOuterRy = 3,
    eHalfPhiSector = 4,
    eAveragePhi = 5,
    eSize = 6
  };

  EllipseBounds() = delete;

  /// Constructor for full of an ellipsoid ring
  ///
  /// @param innerRx The inner ellipse radius in x
  /// @param innerRy The inner ellipse radius in y
  /// @param outerRx The outer ellipse radius in x
  /// @param outerRy The outer ellipse radius in y
  /// @param halfPhi spanning phi sector (is set to pi as default)
  /// @param averagePhi average phi (is set to 0. as default)
  EllipseBounds(double innerRx, double innerRy, double outerRx, double outerRy,
                double halfPhi = std::numbers::pi,
                double averagePhi = 0.) noexcept(false)
      : m_values({innerRx, innerRy, outerRx, outerRy, halfPhi, averagePhi}),
        m_boundingBox(m_values[eInnerRy], m_values[eOuterRy]) {
    checkConsistency();
  }

  /// Constructor - from fixed size array
  ///
  /// @param values The parameter values
  explicit EllipseBounds(const std::array<double, eSize>& values) noexcept(
      false)
      : m_values(values), m_boundingBox(values[eInnerRy], values[eOuterRy]) {
    checkConsistency();
  }

  ~EllipseBounds() override = default;

  BoundsType type() const final;

  /// Return the bound values as dynamically sized vector
  ///
  /// @return this returns a copy of the internal values
  std::vector<double> values() const final;

  /// This method checks if the point given in the local coordinates is between
  /// two ellipsoids if only tol0 is given and additional in the phi sector is
  /// tol1 is given
  ///
  /// @param lposition Local position (assumed to be in right surface frame)
  /// @param boundaryTolerance boundary check directive
  /// @return boolean indicator for the success of this operation
  bool inside(const Vector2& lposition,
              const BoundaryTolerance& boundaryTolerance) const final;

  /// Return the vertices
  ///
  /// @param quarterSegments is the number of segments to approximate a quarter
  /// of a circle. In order to symmetrize fully closed and sectoral cylinders,
  /// also in the first case the two end points are given (albeit they overlap)
  /// in -pi / pi
  ///
  /// @return vector for vertices in 2D
  std::vector<Vector2> vertices(unsigned int quarterSegments) const final;

  // Bounding box representation
  const RectangleBounds& boundingBox() const final;

  /// Output Method for std::ostream
  std::ostream& toStream(std::ostream& sl) const final;

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const { return m_values[bValue]; }

 private:
  std::array<double, eSize> m_values;
  RectangleBounds m_boundingBox;

  /// Check the input values for consistency, will throw a logic_exception
  /// if consistency is not given
  void checkConsistency() noexcept(false);
};

inline std::vector<double> EllipseBounds::values() const {
  std::vector<double> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

inline void EllipseBounds::checkConsistency() noexcept(false) {
  if (get(eInnerRx) >= get(eOuterRx) || get(eInnerRx) < 0. ||
      get(eOuterRx) <= 0.) {
    throw std::invalid_argument("EllipseBounds: invalid along x axis");
  }
  if (get(eInnerRy) >= get(eOuterRy) || get(eInnerRy) < 0. ||
      get(eOuterRy) <= 0.) {
    throw std::invalid_argument("EllipseBounds: invalid along y axis.");
  }
  if (get(eHalfPhiSector) < 0. || get(eHalfPhiSector) > std::numbers::pi) {
    throw std::invalid_argument("EllipseBounds: invalid phi sector setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument("EllipseBounds: invalid phi positioning.");
  }
}

}  // namespace Acts
