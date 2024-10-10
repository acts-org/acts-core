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
#include "Acts/Surfaces/InfiniteBounds.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>
#include <string>

namespace Acts {

class LineBounds;
class DetectorElementBase;
class SurfaceBounds;

///  @class LineSurface
///
///  Base class for a linear surfaces in the TrackingGeometry
///  to describe dirft tube, straw like detectors or the Perigee
///  It inherits from Surface.
///
///  @note It leaves the type() method virtual, so it can not be instantiated
///
/// @image html LineSurface.png
class LineSurface : public Surface {
  friend class Surface;

 protected:
  /// Constructor from Transform3 and bounds
  ///
  /// @param transform The transform that positions the surface in the global
  /// frame
  /// @param radius The straw radius
  /// @param halez The half length in z
  LineSurface(const Transform3& transform, double radius, double halez);

  /// Constructor from Transform3 and a shared bounds object
  ///
  /// @param transform The transform that positions the surface in the global
  /// frame
  /// @param lbounds The bounds describing the straw dimensions, can be
  /// optionally nullptr
  LineSurface(const Transform3& transform,
              std::shared_ptr<const LineBounds> lbounds = nullptr);

  /// Constructor from DetectorElementBase : Element proxy
  ///
  /// @param lbounds The bounds describing the straw dimensions
  /// @param detelement for which this surface is (at least) one representation
  LineSurface(std::shared_ptr<const LineBounds> lbounds,
              const DetectorElementBase& detelement);

  /// Copy constructor
  ///
  /// @param other The source surface for copying
  LineSurface(const LineSurface& other);

  /// Copy constructor - with shift
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param other is the source cone surface
  /// @param shift is the additional transform applied after copying
  LineSurface(const GeometryContext& gctx, const LineSurface& other,
              const Transform3& shift);

 public:
  ~LineSurface() override = default;
  LineSurface() = delete;

  /// Assignment operator
  ///
  /// @param other is the source surface dor copying
  LineSurface& operator=(const LineSurface& other);

  Vector3 normal(const GeometryContext& gctx, const Vector3& pos,
                 const Vector3& direction) const override;

  /// The binning position is the position calculated
  /// for a certain binning type
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param bValue is the binning type to be used
  ///
  /// @return position that can beused for this binning
  Vector3 binningPosition(const GeometryContext& gctx,
                          BinningValue bValue) const final;

  /// Return the measurement frame - this is needed for alignment, in particular
  ///
  /// for StraightLine and Perigee Surface
  ///  - the default implementation is the RotationMatrix3 of the transform
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position where the measurement frame is
  /// constructed
  /// @param direction is the momentum direction used for the measurement frame
  /// construction
  ///
  /// @return is a rotation matrix that indicates the measurement frame
  RotationMatrix3 referenceFrame(const GeometryContext& gctx,
                                 const Vector3& position,
                                 const Vector3& direction) const final;

  /// Calculate the jacobian from local to global which the surface knows best,
  /// hence the calculation is done here.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position global 3D position
  /// @param direction global 3D momentum direction
  ///
  /// @return Jacobian from local to global
  BoundToFreeMatrix boundToFreeJacobian(const GeometryContext& gctx,
                                        const Vector3& position,
                                        const Vector3& direction) const final;

  /// Calculate the derivative of path length at the geometry constraint or
  /// point-of-closest-approach w.r.t. free parameters
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position global 3D position
  /// @param direction global 3D momentum direction
  ///
  /// @return Derivative of path length w.r.t. free parameters
  FreeToPathMatrix freeToPathDerivative(const GeometryContext& gctx,
                                        const Vector3& position,
                                        const Vector3& direction) const final;

  /// Local to global transformation
  ///
  /// @note for line surfaces the momentum direction is used in order to interpret the
  /// drift radius
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param lposition is the local position to be transformed
  /// @param direction is the global momentum direction (used to sign the closest approach)
  ///
  /// @return global position by value
  Vector3 localToGlobal(const GeometryContext& gctx, const Vector2& lposition,
                        const Vector3& direction) const final;

  /// Specified for `LineSurface`: global to local method without dynamic
  /// memory allocation.
  ///
  /// This method is the true global -> local transformation. It makes use of
  /// @c globalToLocal and indicates the sign of the @c Acts::eBoundLoc0
  /// by the given momentum direction.
  ///
  /// The calculation of the sign of the radius (or @f$ d_0 @f$) can be done as
  /// follows:
  /// May @f$ \vec d = \vec m - \vec c @f$ denote the difference between the
  /// center of the line and the global position of the measurement/predicted
  /// state. Then, @f$ \vec d @f$ lies in the so-called measurement plane.
  /// The latter is determined by the two orthogonal vectors @f$
  /// \vec{\texttt{measY}} = \vec{e}_z @f$ and @f$
  /// \vec{\texttt{measX}} = \vec{\texttt{measY}} \times
  /// \frac{\vec{p}}{|\vec{p}|} @f$.
  ///
  /// The sign of the radius (or @f$ d_{0} @f$ ) is then defined by the projection
  /// of @f$ \vec{d} @f$ on @f$ \vec{measX} @f$:<br> @f$ sign = -sign(\vec{d}
  /// \cdot \vec{measX}) @f$
  ///
  /// @image html SignOfDriftCircleD0.gif
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position global 3D position - considered to be on surface but not
  /// inside bounds (check is done)
  /// @param direction global 3D momentum direction (optionally ignored)
  /// @param tolerance (unused)
  ///
  /// @return A `Result<Vector2>`, which is set to `!ok()` if the @p position is not
  /// the point of closest approach to the line surface.
  Result<Vector2> globalToLocal(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction,
      double tolerance = s_onSurfaceTolerance) const final;

  /// Calculate the straight-line intersection with the line surface.
  ///
  /// <b>Mathematical motivation:</b>
  ///
  /// Given two lines in parameteric form:<br>
  ///
  /// @f$ \vec l_{a}(u) = \vec m_a + u \cdot \vec e_{a} @f$
  ///
  /// @f$ \vec l_{b}(\mu) = \vec m_b + \mu \cdot \vec e_{b} @f$
  ///
  /// The vector between any two points on the two lines is given by:
  ///
  /// @f$ \vec s(u, \mu) = \vec l_{b} - l_{a} = \vec m_{ab} + \mu
  /// \cdot
  /// \vec e_{b} - u \cdot \vec e_{a} @f$,
  ///
  /// where @f$ \vec m_{ab} = \vec m_{b} - \vec m_{a} @f$.
  ///
  /// @f$ \vec s(u_0, \mu_0) @f$ denotes the vector between the two
  /// closest points
  ///
  /// @f$ \vec l_{a,0} = l_{a}(u_0) @f$ and @f$ \vec l_{b,0} =
  /// l_{b}(\mu_0) @f$
  ///
  /// and is perpendicular to both, @f$ \vec e_{a} @f$ and @f$ \vec e_{b} @f$.
  ///
  /// This results in a system of two linear equations:
  ///
  /// - (i) @f$ 0 = \vec s(u_0, \mu_0) \cdot \vec e_a = \vec m_{ab} \cdot
  /// \vec e_a + \mu_0 \vec e_a \cdot \vec e_b - u_0 @f$ <br>
  /// - (ii) @f$ 0 = \vec s(u_0, \mu_0) \cdot \vec e_b = \vec m_{ab} \cdot
  /// \vec e_b + \mu_0  - u_0 \vec e_b \cdot \vec e_a @f$ <br>
  ///
  /// Solving (i) and (ii) for @f$ u @f$ and @f$ \mu_0 @f$ yields:
  ///
  /// - @f$ u_0 = \frac{(\vec m_{ab} \cdot \vec e_a)-(\vec m_{ab} \cdot \vec
  /// e_b)(\vec e_a \cdot \vec e_b)}{1-(\vec e_a \cdot \vec e_b)^2} @f$ <br>
  /// - @f$ \mu_0 = - \frac{(\vec m_{ab} \cdot \vec e_b)-(\vec m_{ab} \cdot \vec
  /// e_a)(\vec e_a \cdot \vec e_b)}{1-(\vec e_a \cdot \vec e_b)^2} @f$ <br>
  ///
  /// The function checks if @f$ u_0 \simeq 0@f$ to check if the current @p
  /// position is at the point of closest approach, i.e. the intersection
  /// point, in which case it will return an @c onSurace intersection result.
  /// Otherwise, the path length from @p position to the point of closest
  /// approach (@f$ u_0 @f$) is returned in a @c reachable intersection.
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position The global position as a starting point
  /// @param direction The global direction at the starting point
  ///        @note expected to be normalized
  /// @param boundaryTolerance The boundary check directive for the estimate
  /// @param tolerance the tolerance used for the intersection
  /// @return is the intersection object
  SurfaceMultiIntersection intersect(
      const GeometryContext& gctx, const Vector3& position,
      const Vector3& direction,
      const BoundaryTolerance& boundaryTolerance =
          BoundaryTolerance::Infinite(),
      ActsScalar tolerance = s_onSurfaceTolerance) const final;

  /// the pathCorrection for derived classes with thickness
  /// is by definition 1 for LineSurfaces
  ///
  /// @note input parameters are ignored
  /// @note there's no material associated to the line surface
  double pathCorrection(const GeometryContext& gctx, const Vector3& position,
                        const Vector3& direction) const override;

  /// This method returns the bounds of the surface by reference
  const SurfaceBounds& bounds() const final;

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

  Vector3 lineDirection(const GeometryContext& gctx) const;

 protected:
  std::shared_ptr<const LineBounds> m_bounds;  ///< bounds (shared)

 private:
  /// helper function to apply the globalToLocal with out transform
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param position is the global position
  /// @param direction is the momentum direction
  /// @param lposition is the local position to be filled
  bool globalToLocalPlain(const GeometryContext& gctx, const Vector3& position,
                          const Vector3& direction, Vector2& lposition) const;
};

}  // namespace Acts
