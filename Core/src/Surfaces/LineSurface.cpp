// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/LineSurface.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Surfaces/InfiniteBounds.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/SurfaceError.hpp"
#include "Acts/Surfaces/detail/AlignmentHelper.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/JacobianHelpers.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Acts {
class DetectorElementBase;
}  // namespace Acts

Acts::LineSurface::LineSurface(const Transform3& transform, double radius,
                               double halez)
    : GeometryObject(),
      Surface(transform),
      m_bounds(std::make_shared<const LineBounds>(radius, halez)) {}

Acts::LineSurface::LineSurface(const Transform3& transform,
                               std::shared_ptr<const LineBounds> lbounds)
    : GeometryObject(), Surface(transform), m_bounds(std::move(lbounds)) {}

Acts::LineSurface::LineSurface(std::shared_ptr<const LineBounds> lbounds,
                               const DetectorElementBase& detelement)
    : GeometryObject(), Surface(detelement), m_bounds(std::move(lbounds)) {
  throw_assert(m_bounds, "LineBounds must not be nullptr");
}

Acts::LineSurface::LineSurface(const LineSurface& other)
    : GeometryObject(), Surface(other), m_bounds(other.m_bounds) {}

Acts::LineSurface::LineSurface(const GeometryContext& gctx,
                               const LineSurface& other,
                               const Transform3& shift)
    : GeometryObject(), Surface(gctx, other, shift), m_bounds(other.m_bounds) {}

Acts::LineSurface& Acts::LineSurface::operator=(const LineSurface& other) {
  if (this != &other) {
    Surface::operator=(other);
    m_bounds = other.m_bounds;
  }
  return *this;
}

Acts::Vector3 Acts::LineSurface::localToGlobal(const GeometryContext& gctx,
                                               const Vector2& lposition,
                                               const Vector3& direction) const {
  Vector3 unitZ0 = lineDirection(gctx);

  // get the vector perpendicular to the momentum direction and the straw axis
  Vector3 radiusAxisGlobal = unitZ0.cross(direction);
  Vector3 locZinGlobal =
      transform(gctx) * Vector3(0., 0., lposition[eBoundLoc1]);
  // add eBoundLoc0 * radiusAxis
  return Vector3(locZinGlobal +
                 lposition[eBoundLoc0] * radiusAxisGlobal.normalized());
}

Acts::Result<Acts::Vector2> Acts::LineSurface::globalToLocal(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, double tolerance) const {
  using VectorHelpers::perp;

  // Bring the global position into the local frame. First remove the
  // translation then the rotation.
  Vector3 localPosition = referenceFrame(gctx, position, direction).inverse() *
                          (position - transform(gctx).translation());

  // `localPosition.z()` is not the distance to the PCA but the smallest
  // distance between `position` and the imaginary plane surface defined by the
  // local x,y axes in the global frame and the position of the line surface.
  //
  // This check is also done for the `PlaneSurface` so I aligned the
  // `LineSurface` to do the same thing.
  if (std::abs(localPosition.z()) > std::abs(tolerance)) {
    return Result<Vector2>::failure(SurfaceError::GlobalPositionNotOnSurface);
  }

  // Construct result from local x,y
  Vector2 localXY = localPosition.head<2>();

  return Result<Vector2>::success(localXY);
}

std::string Acts::LineSurface::name() const {
  return "Acts::LineSurface";
}

Acts::RotationMatrix3 Acts::LineSurface::referenceFrame(
    const GeometryContext& gctx, const Vector3& /*position*/,
    const Vector3& direction) const {
  Vector3 unitZ0 = lineDirection(gctx);
  Vector3 unitD0 = unitZ0.cross(direction).normalized();
  Vector3 unitDistance = unitD0.cross(unitZ0);

  RotationMatrix3 mFrame;
  mFrame.col(0) = unitD0;
  mFrame.col(1) = unitZ0;
  mFrame.col(2) = unitDistance;

  return mFrame;
}

double Acts::LineSurface::pathCorrection(const GeometryContext& /*gctx*/,
                                         const Vector3& /*pos*/,
                                         const Vector3& /*mom*/) const {
  return 1.;
}

Acts::Vector3 Acts::LineSurface::binningPosition(
    const GeometryContext& gctx, BinningValue /*bValue*/) const {
  return center(gctx);
}

Acts::Vector3 Acts::LineSurface::normal(const GeometryContext& gctx,
                                        const Vector3& pos,
                                        const Vector3& direction) const {
  auto ref = referenceFrame(gctx, pos, direction);
  return ref.col(2);
}

const Acts::SurfaceBounds& Acts::LineSurface::bounds() const {
  if (m_bounds) {
    return (*m_bounds.get());
  }
  return s_noBounds;
}

Acts::SurfaceMultiIntersection Acts::LineSurface::intersect(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, const BoundaryTolerance& boundaryTolerance,
    ActsScalar tolerance) const {
  // The nomenclature is following the header file and doxygen documentation

  const Vector3& ma = position;
  const Vector3& ea = direction;

  // Origin of the line surface
  Vector3 mb = transform(gctx).translation();
  // Line surface axis
  Vector3 eb = lineDirection(gctx);

  // Now go ahead and solve for the closest approach
  Vector3 mab = mb - ma;
  double eaTeb = ea.dot(eb);
  double denom = 1 - eaTeb * eaTeb;

  // `tolerance` does not really have a meaning here it is just a sufficiently
  // small number so `u` does not explode
  if (std::abs(denom) < std::abs(tolerance)) {
    // return a false intersection
    return {{Intersection3D::invalid(), Intersection3D::invalid()}, this};
  }

  double u = (mab.dot(ea) - mab.dot(eb) * eaTeb) / denom;
  // Check if we are on the surface already
  Intersection3D::Status status = std::abs(u) > std::abs(tolerance)
                                      ? Intersection3D::Status::reachable
                                      : Intersection3D::Status::onSurface;
  Vector3 result = ma + u * ea;
  // Evaluate the boundary check if requested
  // m_bounds == nullptr prevents unnecessary calculations for PerigeeSurface
  if (m_bounds && !boundaryTolerance.isInfinite()) {
    Vector3 vecLocal = result - mb;
    double cZ = vecLocal.dot(eb);
    double cR = (vecLocal - cZ * eb).norm();
    if (!m_bounds->inside({cR, cZ}, boundaryTolerance)) {
      status = Intersection3D::Status::missed;
    }
  }

  return {{Intersection3D(result, u, status), Intersection3D::invalid()}, this};
}

Acts::BoundToFreeMatrix Acts::LineSurface::boundToFreeJacobian(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  assert(isOnSurface(gctx, position, direction, BoundaryTolerance::Infinite()));

  // retrieve the reference frame
  auto rframe = referenceFrame(gctx, position, direction);

  Vector2 local = *globalToLocal(gctx, position, direction,
                                 std::numeric_limits<double>::max());

  // For the derivative of global position with bound angles, refer the
  // following white paper:
  // https://acts.readthedocs.io/en/latest/white_papers/line-surface-jacobian.html

  BoundToFreeMatrix jacToGlobal =
      Surface::boundToFreeJacobian(gctx, position, direction);

  // the projection of direction onto ref frame normal
  double ipdn = 1. / direction.dot(rframe.col(2));
  // build the cross product of d(D)/d(eBoundPhi) components with y axis
  Vector3 dDPhiY = rframe.block<3, 1>(0, 1).cross(
      jacToGlobal.block<3, 1>(eFreeDir0, eBoundPhi));
  // and the same for the d(D)/d(eTheta) components
  Vector3 dDThetaY = rframe.block<3, 1>(0, 1).cross(
      jacToGlobal.block<3, 1>(eFreeDir0, eBoundTheta));
  // and correct for the x axis components
  dDPhiY -= rframe.block<3, 1>(0, 0) * (rframe.block<3, 1>(0, 0).dot(dDPhiY));
  dDThetaY -=
      rframe.block<3, 1>(0, 0) * (rframe.block<3, 1>(0, 0).dot(dDThetaY));
  // set the jacobian components for global d/ phi/Theta
  jacToGlobal.block<3, 1>(eFreePos0, eBoundPhi) = dDPhiY * local.x() * ipdn;
  jacToGlobal.block<3, 1>(eFreePos0, eBoundTheta) = dDThetaY * local.x() * ipdn;

  return jacToGlobal;
}

Acts::FreeToPathMatrix Acts::LineSurface::freeToPathDerivative(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  assert(isOnSurface(gctx, position, direction, BoundaryTolerance::Infinite()));

  // The vector between position and center
  Vector3 pcRowVec = position - center(gctx);
  // The local frame z axis
  Vector3 localZAxis = lineDirection(gctx);
  // The local z coordinate
  double pz = pcRowVec.dot(localZAxis);
  // Cosine of angle between momentum direction and local frame z axis
  double dz = localZAxis.dot(direction);
  double norm = 1 / (1 - dz * dz);

  // Initialize the derivative of propagation path w.r.t. free parameter
  FreeToPathMatrix freeToPath = FreeToPathMatrix::Zero();

  // The derivative of path w.r.t. position
  freeToPath.segment<3>(eFreePos0) =
      norm * (dz * localZAxis.transpose() - direction.transpose());

  // The derivative of path w.r.t. direction
  freeToPath.segment<3>(eFreeDir0) =
      norm * (pz * localZAxis.transpose() - pcRowVec.transpose());

  return freeToPath;
}

Acts::AlignmentToPathMatrix Acts::LineSurface::alignmentToPathDerivative(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  assert(isOnSurface(gctx, position, direction, BoundaryTolerance::Infinite()));

  // The vector between position and center
  Vector3 pcRowVec = position - center(gctx);
  // The local frame z axis
  Vector3 localZAxis = lineDirection(gctx);
  // The local z coordinate
  double pz = pcRowVec.dot(localZAxis);
  // Cosine of angle between momentum direction and local frame z axis
  double dz = localZAxis.dot(direction);
  double norm = 1 / (1 - dz * dz);
  // Calculate the derivative of local frame axes w.r.t its rotation
  auto [rotToLocalXAxis, rotToLocalYAxis, rotToLocalZAxis] =
      detail::rotationToLocalAxesDerivative(transform(gctx).rotation());

  // Initialize the derivative of propagation path w.r.t. local frame
  // translation (origin) and rotation
  AlignmentToPathMatrix alignToPath = AlignmentToPathMatrix::Zero();
  alignToPath.segment<3>(eAlignmentCenter0) =
      norm * (direction.transpose() - dz * localZAxis.transpose());
  alignToPath.segment<3>(eAlignmentRotation0) =
      norm * (dz * pcRowVec.transpose() + pz * direction.transpose()) *
      rotToLocalZAxis;

  return alignToPath;
}

Acts::ActsMatrix<2, 3> Acts::LineSurface::localCartesianToBoundLocalDerivative(
    const GeometryContext& gctx, const Vector3& position) const {
  // calculate the transformation to local coordinates
  Vector3 localPosition = transform(gctx).inverse() * position;
  double localPhi = VectorHelpers::phi(localPosition);

  ActsMatrix<2, 3> loc3DToLocBound = ActsMatrix<2, 3>::Zero();
  loc3DToLocBound << std::cos(localPhi), std::sin(localPhi), 0, 0, 0, 1;

  return loc3DToLocBound;
}

Acts::Vector3 Acts::LineSurface::lineDirection(
    const GeometryContext& gctx) const {
  return transform(gctx).linear().col(2);
}
