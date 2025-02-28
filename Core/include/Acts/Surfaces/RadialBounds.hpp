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
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <array>
#include <iosfwd>
#include <numbers>
#include <vector>

namespace Acts {

/// @class RadialBounds
///
/// Class to describe the bounds for a planar DiscSurface.
/// By providing an argument for hphisec, the bounds can
/// be restricted to a phi-range around the center position.
///
class RadialBounds : public DiscBounds {
 public:
  enum BoundValues {
    eMinR = 0,
    eMaxR = 1,
    eHalfPhiSector = 2,
    eAveragePhi = 3,
    eSize = 4
  };

  /// Constructor for full disc of symmetric disc around phi=0
  ///
  /// @param minR The inner radius (0 for full disc)
  /// @param maxR The outer radius
  /// @param halfPhi The half opening angle (Pi for full angular coverage)
  /// @param avgPhi The average phi for the disc/ring sector
  explicit RadialBounds(double minR, double maxR,
                        double halfPhi = std::numbers::pi,
                        double avgPhi = 0.) noexcept(false)
      : m_values({minR, maxR, halfPhi, avgPhi}) {
    checkConsistency();
  }

  /// Constructor from array values
  ///
  /// @param values The bound values
  explicit RadialBounds(const std::array<double, eSize>& values) noexcept(false)
      : m_values(values) {
    checkConsistency();
  }

  BoundsType type() const final { return SurfaceBounds::eDisc; }

  /// Return the bound values as dynamically sized vector
  ///
  /// @return this returns a copy of the internal values
  std::vector<double> values() const final;

  /// For disc surfaces the local position in (r,phi) is checked
  ///
  /// @param lposition local position to be checked
  /// @param boundaryTolerance boundary check directive
  ///
  /// @return is a boolean indicating the operation success
  bool inside(const Vector2& lposition,
              const BoundaryTolerance& boundaryTolerance) const final;

  /// Outstream operator
  ///
  /// @param sl is the ostream to be dumped into
  std::ostream& toStream(std::ostream& sl) const final;

  /// Return method for inner Radius
  double rMin() const final { return get(eMinR); }

  /// Return method for outer Radius
  double rMax() const final { return get(eMaxR); }

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const { return m_values[bValue]; }

  /// Returns true for full phi coverage
  bool coversFullAzimuth() const final {
    return (get(eHalfPhiSector) == std::numbers::pi);
  }

  /// Checks if this is inside the radial coverage
  /// given the a tolerance
  bool insideRadialBounds(double R, double tolerance = 0.) const final {
    return (R + tolerance > get(eMinR) && R - tolerance < get(eMaxR));
  }

  /// Return a reference radius for binning
  double binningValueR() const final { return 0.5 * (get(eMinR) + get(eMaxR)); }

  /// Return a reference radius for binning
  double binningValuePhi() const final { return get(eAveragePhi); }

 private:
  std::array<double, eSize> m_values;

  /// Check the input values for consistency, will throw a logic_exception
  /// if consistency is not given
  void checkConsistency() noexcept(false);

  /// Private helper method to shift a local position
  /// within the bounds
  ///
  /// @param lposition The local position in polar coordinates
  Vector2 shifted(const Vector2& lposition) const;

  /// This method returns the xy coordinates of vertices along
  /// the radial bounds
  ///
  /// @param lseg the number of segments used to approximate
  /// and eventually curved line
  ///
  /// @note that the extremas are given, which may slightly alter the
  /// number of segments returned
  ///
  /// @return vector for vertices in 2D
  std::vector<Vector2> vertices(unsigned int lseg) const final;
};

}  // namespace Acts
