// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <array>
#include <cmath>
#include <exception>
#include <iosfwd>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace Acts {

/// @brief Class that implements a (potentially asymmetric) bounds with
/// difference between surface bound center and surface coordinate center
///
/// These bounds combine two different systems:
///  * module system : radial bounds centred on the moduleOrigin
///  * strip system : phi bounds centred on the stripOrigin
///
/// The measurement will be done in the strip system, with r/phi local
/// coordinates.
///
class AnnulusBounds : public DiscBounds {
 public:
  enum BoundValues : int {
    eMinR = 0,
    eMaxR = 1,
    eMinPhiRel = 2,
    eMaxPhiRel = 3,
    eAveragePhi = 4,
    eOriginX = 5,
    eOriginY = 6,
    eSize = 7
  };

  AnnulusBounds() = delete;

  /// @brief Default constructor from parameters
  /// @param minR inner radius, in module system
  /// @param maxR outer radius, in module system
  /// @param minPhiRel right angular edge, in strip system, rel to avgPhi
  /// @param maxPhiRel left angular edge, in strip system, rel to avgPhi
  /// @param moduleOrigin The origin offset between the two systems.
  /// @param avgPhi (Optional) internal rotation of this bounds object's local
  /// frame
  /// @note For @c morigin you need to actually calculate the cartesian
  /// offset
  AnnulusBounds(double minR, double maxR, double minPhiRel, double maxPhiRel,
                const Vector2& moduleOrigin = {0, 0},
                double avgPhi = 0) noexcept(false)
      : AnnulusBounds({minR, maxR, minPhiRel, maxPhiRel, avgPhi,
                       moduleOrigin.x(), moduleOrigin.y()}) {}

  /// Constructor - from parameters array
  ///
  /// @param values The parameter array
  AnnulusBounds(const std::array<double, eSize>& values) noexcept(false);

  AnnulusBounds(const AnnulusBounds& source) = default;

  SurfaceBounds::BoundsType type() const final;

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

  /// Outstream operator
  ///
  /// @param sl is the ostream to be dumped into
  std::ostream& toStream(std::ostream& sl) const final;

  /// Access to the bound values
  /// @param bValue the class nested enum for the array access
  double get(BoundValues bValue) const { return m_values[bValue]; }

  /// @brief Returns the right angular edge of the module
  /// @return The right side angle
  double phiMin() const;

  /// @brief Returns the left angular edge of the module
  /// @return The left side angle
  double phiMax() const;

  /// Returns true for full phi coverage
  bool coversFullAzimuth() const final;

  /// Checks if this is inside the radial coverage
  /// given the a tolerance
  bool insideRadialBounds(double R, double tolerance = 0.) const final;

  /// Return a reference radius for binning
  double binningValueR() const final;

  /// Return a reference radius for binning
  double binningValuePhi() const final;

  /// @brief Returns moduleOrigin, but rotated out, so @c averagePhi is already
  /// considered. The module origin needs to consider the rotation introduced by
  /// @c averagePhi
  /// @return The origin of the local frame
  Vector2 moduleOrigin() const;

  /// This method returns the four corners of the bounds in polar coordinates
  /// Starting from the upper right (max R, pos locX) and proceeding clock-wise
  /// i.e. (max R; pos locX), (min R; pos locX), (min R; neg loc X), (max R: neg
  /// locX)
  std::vector<Vector2> corners() const;

  /// This method returns the xy coordinates of the four corners of the
  /// bounds in module coordinates (in x/y), and if quarterSegments is bigger or
  /// equal to 0, the curved part of the segment is included and approximated
  /// by the corresponding number of segments.
  ///
  /// Starting from the upper right (max R, pos locX) and proceeding clock-wise
  /// i.e. (max R; pos locX), (min R; pos locX), (min R; neg loc X), (max R: neg
  /// locX)
  ///
  /// @param quarterSegments the number of segments used to approximate
  /// a quarter of a circle
  ///
  /// @return vector for vertices in 2D
  std::vector<Vector2> vertices(
      unsigned int quarterSegments = 2u) const override;

  /// This method returns inner radius
  double rMin() const final;

  /// This method returns outer radius
  double rMax() const final;

 private:
  std::array<double, eSize> m_values;

  // @TODO: Does this need to be in bound values?
  Vector2 m_moduleOrigin;
  Vector2 m_shiftXY;  // == -m_moduleOrigin
  Vector2 m_shiftPC;
  Transform2 m_rotationStripPC;
  Transform2 m_translation;

  // Vectors needed for inside checking
  Vector2 m_outLeftStripPC;
  Vector2 m_inLeftStripPC;
  Vector2 m_outRightStripPC;
  Vector2 m_inRightStripPC;

  Vector2 m_outLeftModulePC;
  Vector2 m_inLeftModulePC;
  Vector2 m_outRightModulePC;
  Vector2 m_inRightModulePC;

  Vector2 m_outLeftStripXY;
  Vector2 m_inLeftStripXY;
  Vector2 m_outRightStripXY;
  Vector2 m_inRightStripXY;

  /// Check the input values for consistency, will throw a logic_exception
  /// if consistency is not given
  void checkConsistency() noexcept(false);

  /// Inside check for the bounds object driven by the boundary check directive
  /// Each Bounds has a method inside, which checks if a LocalPosition is inside
  /// the bounds  Inside can be called without/with tolerances.
  ///
  /// @param lposition Local position (assumed to be in right surface frame)
  /// @param tolR tolerance on the radius
  /// @param tolPhi tolerance on the polar angle phi
  /// @return boolean indicator for the success of this operation
  virtual bool inside(const Vector2& lposition, double tolR,
                      double tolPhi) const final;
   
  /// Transform the strip cartesian
  /// into the module polar system
  ///
  /// @param vStripXY the position in the cartesian strip system
  /// @return the position in the module polar coordinate system
  Vector2 stripXYToModulePC(const Vector2& vStripXY) const;

  /// Private helper method
  Vector2 closestOnSegment(const Vector2& a, const Vector2& b, const Vector2& p,
                           const SquareMatrix2& weight) const;

  /// Private helper method
  double squaredNorm(const Vector2& v, const SquareMatrix2& weight) const;

private:
  bool inside(const Vector2& lposition, double tolR,
              double tolPhiR,
              double scale) const;
};

inline SurfaceBounds::BoundsType AnnulusBounds::type() const {
  return SurfaceBounds::eAnnulus;
}

inline double AnnulusBounds::rMin() const {
  return get(eMinR);
}

inline double AnnulusBounds::rMax() const {
  return get(eMaxR);
}

inline double AnnulusBounds::phiMin() const {
  return get(eMinPhiRel) + get(eAveragePhi);
}

inline double AnnulusBounds::phiMax() const {
  return get(eMaxPhiRel) + get(eAveragePhi);
}

inline bool AnnulusBounds::coversFullAzimuth() const {
  return (std::abs((get(eMinPhiRel) - get(eMaxPhiRel)) - std::numbers::pi) <
          s_onSurfaceTolerance);
}

inline bool AnnulusBounds::insideRadialBounds(double R,
                                              double tolerance) const {
  return ((R + tolerance) > get(eMinR) && (R - tolerance) < get(eMaxR));
}

inline double AnnulusBounds::binningValueR() const {
  return 0.5 * (get(eMinR) + get(eMaxR));
}

inline double AnnulusBounds::binningValuePhi() const {
  return get(eAveragePhi);
}

inline std::vector<double> AnnulusBounds::values() const {
  std::vector<double> valvector;
  valvector.insert(valvector.begin(), m_values.begin(), m_values.end());
  return valvector;
}

inline void AnnulusBounds::checkConsistency() noexcept(false) {
  if (get(eMinR) < 0. || get(eMaxR) < 0. || get(eMinR) > get(eMaxR) ||
      std::abs(get(eMinR) - get(eMaxR)) < s_epsilon) {
    throw std::invalid_argument("AnnulusBounds: invalid radial setup.");
  }
  if (get(eMinPhiRel) != detail::radian_sym(get(eMinPhiRel)) ||
      get(eMaxPhiRel) != detail::radian_sym(get(eMaxPhiRel)) ||
      get(eMinPhiRel) > get(eMaxPhiRel)) {
    throw std::invalid_argument("AnnulusBounds: invalid phi boundary setup.");
  }
  if (get(eAveragePhi) != detail::radian_sym(get(eAveragePhi))) {
    throw std::invalid_argument("AnnulusBounds: invalid phi positioning.");
  }
}

}  // namespace Acts
