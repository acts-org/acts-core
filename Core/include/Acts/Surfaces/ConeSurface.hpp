// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Alignment.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceConcept.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Result.hpp"
#include "Acts/Utilities/detail/RealQuadraticEquation.hpp"

#include <cmath>
#include <cstddef>
#include <memory>
#include <string>

namespace Acts {

/// @class ConeSurface
///
/// Class for a conical surface in the Tracking geometry.
/// It inherits from Surface.
///
/// The ConeSurface is special since no corresponding
/// Track parameters exist since they're numerical instable
/// at the tip of the cone.
/// Propagations to a cone surface will be returned in
/// curvilinear coordinates.

class ConeSurface : public RegularSurface {
  friend class Surface;

 protected:
  /// Constructor form HepTransform and an opening angle
  ///
  /// @param transform is the transform to place to cone in a 3D frame
  /// @param alpha is the opening angle of the cone
  /// @param symmetric indicates if the cones are built to +/1 z
  ConeSurface(const Transform3& transform, double alpha,
              bool symmetric = false);

  /// Constructor form HepTransform and an opening angle
  ///
  /// @param transform is the transform that places the cone in the global frame
  /// @param alpha is the opening angle of the cone
  /// @param zmin is the z range over which the cone spans
  /// @param zmax is the z range over which the cone spans
  /// @param halfPhi is the opening angle for cone ssectors
  ConeSurface(const Transform3& transform, double alpha, double zmin,
              double zmax, double halfPhi = M_PI);

  /// Constructor from HepTransform and ConeBounds
  ///
  /// @param transform is the transform that places the cone in the global frame
  /// @param cbounds is the boundary class, the bounds must exit
  ConeSurface(const Transform3& transform,
              std::shared_ptr<const ConeBounds> cbounds);

  /// Copy constructor
  ///
  /// @param other is the source cone surface
  ConeSurface(const ConeSurface& other);

  /// Copy constructor - with shift
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param other is the source cone surface
  /// @param shift is the additional transform applied after copying
  ConeSurface(const GeometryContext& gctx, const ConeSurface& other,
              const Transform3& shift);

 public:
  ~ConeSurface() override = default;
  ConeSurface() = delete;

  /// Assignment operator
  ///
  /// @param other is the source surface for the assignment
  ConeSurface& operator=(const ConeSurface& other);

  /// The binning position method - is overloaded for r-type binning
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param bValue defines the type of binning applied in the global frame
  ///
  /// @return The return type is a vector for positioning in the global frame
  Vector3 binningPosition(const GeometryContext& gctx,
                          BinningValue bValue) const final;

  /// Return the surface type
  SurfaceType type() const override;

  /// Return the measurement frame - this is needed for alignment, in particular
  ///  for StraightLine and Perigee Surface
  ///  - the default implementation is the RotationMatrix3 of the transform
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position where the measurement frame is
  /// constructed
  /// @param direction is the momentum direction used for the measurement frame
  /// construction
  /// @return matrix that indicates the measurement frame
  RotationMatrix3 referenceFrame(const GeometryContext& gctx,
                                 const Vector3& position,
                                 const Vector3& direction) const final;

  /// Return method for surface normal information
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lposition is the local position at normal vector request
  /// @return Vector3 normal vector in global frame
  Vector3 normal(const GeometryContext& gctx,
                 const Vector2& lposition) const final;

  /// Return method for surface normal information
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position as normal vector base
  /// @return Vector3 normal vector in global frame
  Vector3 normal(const GeometryContext& gctx,
                 const Vector3& position) const final;

  // Return method for the rotational symmetry axis
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  ///
  // @return This returns the local z axis
  virtual Vector3 rotSymmetryAxis(const GeometryContext& gctx) const;

  /// This method returns the ConeBounds by reference
  const ConeBounds& bounds() const final;

  /// Local to global transformation
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lposition is the local position to be transformed
  ///
  /// @return The global position by value
  Vector3 localToGlobal(const GeometryContext& gctx,
                        const Vector2& lposition) const final;

  // Use overloads from `RegularSurface`
  using RegularSurface::globalToLocal;
  using RegularSurface::localToGlobal;
  using RegularSurface::normal;

  /// Global to local transformation
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position to be transformed
  /// @param tolerance optional tolerance within which a point is considered
  /// valid on surface
  ///
  /// @return a Result<Vector2> which can be !ok() if the operation fails
  Result<Vector2> globalToLocal(
      const GeometryContext& gctx, const Vector3& position,
      double tolerance = s_onSurfaceTolerance) const final;

  /// Straight line intersection schema from position/direction
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The position to start from
  /// @param direction The direction at start
  /// @param boundaryTolerance the Boundary Check
  /// @param tolerance the tolerance used for the intersection
  ///
  /// If possible returns both solutions for the cylinder
  ///
  /// @return @c SurfaceMultiIntersection object (contains intersection & surface)
  SurfaceMultiIntersection intersect(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction,
      const BoundaryTolerance& boundaryTolerance =
          BoundaryTolerance::Infinite(),
      double tolerance = s_onSurfaceTolerance) const final;

  /// The pathCorrection for derived classes with thickness
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global potion at the correction point
  /// @param direction is the momentum direction at the correction point
  /// @return is the path correction due to incident angle
  double pathCorrection(const GeometryContext& gctx, const Vector3& position,
                        const Vector3& direction) const final;

  /// Return a Polyhedron for the surfaces
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lseg Number of segments along curved lines, it represents
  /// the full 2*M_PI coverange, if lseg is set to 1 only the extrema
  /// are given
  /// @note that a surface transform can invalidate the extrema
  /// in the transformed space
  ///
  /// @return A list of vertices and a face/facett description of it
  Polyhedron polyhedronRepresentation(const GeometryContext& gctx,
                                      std::size_t lseg) const override;

  /// Return properly formatted class name for screen output
  std::string name() const override;

  /// Calculate the derivative of path length at the geometry constraint or
  /// point-of-closest-approach w.r.t. alignment parameters of the surface (i.e.
  /// local frame origin in global 3D Cartesian coordinates and its rotation
  /// represented with extrinsic Euler angles)
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position global 3D position
  /// @param direction global 3D momentum direction
  ///
  /// @return Derivative of path length w.r.t. the alignment parameters
  AlignmentToPathMatrix alignmentToPathDerivative(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction) const final;

  /// Calculate the derivative of bound track parameters local position w.r.t.
  /// position in local 3D Cartesian coordinates
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The position of the parameters in global
  ///
  /// @return Derivative of bound local position w.r.t. position in local 3D
  /// cartesian coordinates
  ActsMatrix<2, 3> localCartesianToBoundLocalDerivative(
      const GeometryContext& gctx, const Vector3& position) const final;

 protected:
  std::shared_ptr<const ConeBounds> m_bounds;  ///< bounds (shared)

 private:
  /// Implementation of the intersection solver
  ///
  /// <b>mathematical motivation:</b>
  ///
  ///   The calculation will be done in the 3-dim frame of the cone,
  ///   i.e. the symmetry axis of the cone is the z-axis, x- and y-axis are
  /// perpendicular
  ///   to the z-axis. In this frame the cone is centered around the origin.
  ///   Therefore the two points describing the line have to be first
  ///   recalculated
  /// into the new frame.
  ///   Suppose, this is done, the points of intersection can be
  ///   obtained as follows:<br>
  ///
  ///   The cone is described by the implicit equation
  ///   @f$x^2 + y^2 = z^2 \tan \alpha@f$
  ///   where @f$\alpha@f$ is opening half-angle of the cone  the and
  ///   the line by the parameter equation (with @f$t@f$ the
  ///   parameter and @f$x_1@f$ and @f$x_2@f$ are points on the line)
  ///   @f$(x,y,z) = \vec x_1 + (\vec x_2 - \vec x_2) t @f$.
  ///   The intersection is the given to the value of @f$t@f$ where
  ///   the @f$(x,y,z)@f$ coordinates of the line satisfy the implicit
  ///   equation of the cone. Inserting the expression for the points
  ///   on the line into the equation of the cone and rearranging to
  ///   the form of a  gives (letting @f$ \vec x_d = \frac{\vec x_2 - \vec
  ///   x_1}{|\vec x_2 - \vec x_1|} @f$):
  ///   @f$t^2 (x_d^2 + y_d^2 - z_d^2 \tan^2 \alpha) + 2 t (x_1 x_d +
  ///   y_1 y_d - z_1 z_d \tan^2 \alpha) + (x_1^2 + y_1^2 - z_1^2
  ///   \tan^2 \alpha) = 0 @f$
  ///   Solving the above for @f$t@f$ and putting the values into the
  ///   equation of the line gives the points of intersection. @f$t@f$
  ///   is also the length of the path, since we normalized @f$x_d@f$
  ///   to be unit length.
  ///
  /// @return the quadratic equation
  detail::RealQuadraticEquation intersectionSolver(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction) const;
};

static_assert(RegularSurfaceConcept<ConeSurface>,
              "ConeSurface does not fulfill RegularSurfaceConcept");

}  // namespace Acts
