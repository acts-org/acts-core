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

#include <iosfwd>
#include <memory>
#include <string>

namespace Acts {

class Surface;
class PlaneSurface;

/// @brief Utility class for curvilinear surfaces
///
class CurvilinearSurface final {
 public:
  explicit CurvilinearSurface(const Vector3& direction)
      : m_direction{direction} {}

  /// Constructor with direction vector
  CurvilinearSurface(const Vector3& position, const Vector3& direction)
      : m_position{position}, m_direction{direction} {}

  /// Return method for the surface center by reference
  /// @note the center is always recalculated in order to not keep a cache
  /// @return center position by value
  Vector3 center() const { return m_position; }

  /// Return the surface normal at a given @p position and @p direction.
  /// This method is fully generic, and valid for all surface types.
  /// @note For some surface types, the @p direction is ignored, but
  ///       it is **not safe** to pass in a zero vector!
  /// @return The normal vector at the given position and direction
  Vector3 normal() const { return m_direction; }

  bool isStandardRepresentation() const;

  /// Return method for the reference frame
  /// This is the frame in which the covariance matrix is defined (specialized
  /// by all surfaces)
  ///
  /// @return RotationMatrix3 which defines the three axes of the measurement
  /// frame
  RotationMatrix3 referenceFrame() const;

  /// Return method for the surface Transform3 by reference
  /// In case a detector element is associated the surface transform
  /// is just forwarded to the detector element in order to keep the
  /// (mis-)alignment cache cetrally handled
  ///
  /// @return the contextual transform
  Transform3 transform() const;

  /// Calculate the jacobian from local to global which the surface knows best,
  /// hence the calculation is done here.
  ///
  /// @return Jacobian from local to global
  BoundToFreeMatrix boundToFreeJacobian() const;

  /// Calculate the jacobian from global to local which the surface knows best,
  /// hence the calculation is done here.
  ///
  /// @return Jacobian from global to local
  FreeToBoundMatrix freeToBoundJacobian() const;

  /// Calculate the derivative of path length at the geometry constraint or
  /// point-of-closest-approach w.r.t. free parameters. The calculation is
  /// identical for all surfaces where the reference frame does not depend on
  /// the direction
  ///
  /// @return Derivative of path length w.r.t. free parameters
  FreeToPathMatrix freeToPathDerivative() const;

  /// Output Method for std::ostream
  ///
  /// @param sl is the ostream to be dumped into
  std::ostream& toStream(std::ostream& sl) const;

  /// Output into a std::string
  ///
  /// @return the string representation of the curvilinear surface
  std::string toString() const;

  /// Return the plane surface representation of the curvilinear surface
  ///
  /// @return the plane surface representation of the curvilinear surface
  std::shared_ptr<PlaneSurface> planeSurface() const;

  /// Return the surface representation of the curvilinear surface
  ///
  /// @note same as planeSurface() but returns the base class.
  ///   This is useful if the type of the surface is not relevant.
  ///
  /// @return the surface representation of the curvilinear surface
  std::shared_ptr<Surface> surface() const;

  /// Output operator
  ///
  /// @param os is the ostream to be dumped into
  /// @param surface is the CurvilinearSurface to be dumped
  /// @return the ostream
  friend std::ostream& operator<<(std::ostream& os,
                                  const CurvilinearSurface& surface);

 private:
  Vector3 m_position = Vector3::Zero();
  Vector3 m_direction = Vector3::UnitZ();
};

}  // namespace Acts
