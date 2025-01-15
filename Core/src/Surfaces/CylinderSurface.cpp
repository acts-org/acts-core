// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/CylinderSurface.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/SurfaceError.hpp"
#include "Acts/Surfaces/SurfaceMergingException.hpp"
#include "Acts/Surfaces/detail/AlignmentHelper.hpp"
#include "Acts/Surfaces/detail/FacesHelper.hpp"
#include "Acts/Surfaces/detail/MergeHelper.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts {

using VectorHelpers::perp;
using VectorHelpers::phi;

CylinderSurface::CylinderSurface(const CylinderSurface& other)
    : GeometryObject(), RegularSurface(other), m_bounds(other.m_bounds) {}

CylinderSurface::CylinderSurface(const GeometryContext& gctx,
                                 const CylinderSurface& other,
                                 const Transform3& shift)
    : GeometryObject(),
      RegularSurface(gctx, other, shift),
      m_bounds(other.m_bounds) {}

CylinderSurface::CylinderSurface(const Transform3& transform, double radius,
                                 double halfz, double halfphi, double avphi,
                                 double bevelMinZ, double bevelMaxZ)
    : RegularSurface(transform),
      m_bounds(std::make_shared<const CylinderBounds>(
          radius, halfz, halfphi, avphi, bevelMinZ, bevelMaxZ)) {}

CylinderSurface::CylinderSurface(std::shared_ptr<const CylinderBounds> cbounds,
                                 const DetectorElementBase& detelement)
    : RegularSurface(detelement), m_bounds(std::move(cbounds)) {
  // surfaces representing a detector element must have bounds
  throw_assert(m_bounds, "CylinderBounds must not be nullptr");
}

CylinderSurface::CylinderSurface(const Transform3& transform,
                                 std::shared_ptr<const CylinderBounds> cbounds)
    : RegularSurface(transform), m_bounds(std::move(cbounds)) {
  throw_assert(m_bounds, "CylinderBounds must not be nullptr");
}

CylinderSurface& CylinderSurface::operator=(const CylinderSurface& other) {
  if (this != &other) {
    Surface::operator=(other);
    m_bounds = other.m_bounds;
  }
  return *this;
}

// return the binning position for ordering in the BinnedArray
Vector3 CylinderSurface::referencePosition(const GeometryContext& gctx,
                                           AxisDirection aDir) const {
  // special binning type for R-type methods
  if (aDir == AxisDirection::AxisR || aDir == AxisDirection::AxisRPhi) {
    double R = bounds().get(CylinderBounds::eR);
    double phi = bounds().get(CylinderBounds::eAveragePhi);
    return localToGlobal(gctx, Vector2{phi * R, 0}, Vector3{});
  }
  // give the center as default for all of these binning types
  // AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ,
  // AxisDirection::AxisR, AxisDirection::AxisPhi, AxisDirection::AxisRPhi,
  // AxisDirection::AxisTheta, AxisDirection::AxisEta
  return center(gctx);
}

// return the measurement frame: it's the tangential plane
RotationMatrix3 CylinderSurface::referenceFrame(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& /*direction*/) const {
  RotationMatrix3 mFrame;
  // construct the measurement frame
  // measured Y is the z axis
  Vector3 measY = rotSymmetryAxis(gctx);
  // measured z is the position normalized transverse (in local)
  Vector3 measDepth = normal(gctx, position);
  // measured X is what comoes out of it
  Vector3 measX(measY.cross(measDepth).normalized());
  // assign the columnes
  mFrame.col(0) = measX;
  mFrame.col(1) = measY;
  mFrame.col(2) = measDepth;
  // return the rotation matrix
  return mFrame;
}

Surface::SurfaceType CylinderSurface::type() const {
  return Surface::Cylinder;
}

Vector3 CylinderSurface::localToGlobal(const GeometryContext& gctx,
                                       const Vector2& lposition) const {
  // create the position in the local 3d frame
  double r = bounds().get(CylinderBounds::eR);
  double phi = lposition[0] / r;
  Vector3 position(r * cos(phi), r * sin(phi), lposition[1]);
  return transform(gctx) * position;
}

Result<Vector2> CylinderSurface::globalToLocal(const GeometryContext& gctx,
                                               const Vector3& position,
                                               double tolerance) const {
  double inttol = tolerance;
  if (tolerance == s_onSurfaceTolerance) {
    // transform default value!
    // @TODO: check if s_onSurfaceTolerance would do here
    inttol = bounds().get(CylinderBounds::eR) * 0.0001;
  }
  if (inttol < 0.01) {
    inttol = 0.01;
  }
  const Transform3& sfTransform = transform(gctx);
  Transform3 inverseTrans(sfTransform.inverse());
  Vector3 loc3Dframe(inverseTrans * position);
  if (std::abs(perp(loc3Dframe) - bounds().get(CylinderBounds::eR)) > inttol) {
    return Result<Vector2>::failure(SurfaceError::GlobalPositionNotOnSurface);
  }
  return Result<Vector2>::success(
      {bounds().get(CylinderBounds::eR) * phi(loc3Dframe), loc3Dframe.z()});
}

std::string CylinderSurface::name() const {
  return "Acts::CylinderSurface";
}

Vector3 CylinderSurface::normal(const GeometryContext& gctx,
                                const Vector2& lposition) const {
  double phi = lposition[0] / m_bounds->get(CylinderBounds::eR);
  Vector3 localNormal(cos(phi), sin(phi), 0.);
  return transform(gctx).linear() * localNormal;
}

Vector3 CylinderSurface::normal(const GeometryContext& gctx,
                                const Vector3& position) const {
  const Transform3& sfTransform = transform(gctx);
  // get it into the cylinder frame
  Vector3 pos3D = sfTransform.inverse() * position;
  // set the z coordinate to 0
  pos3D.z() = 0.;
  // normalize and rotate back into global
  return sfTransform.linear() * pos3D.normalized();
}

double CylinderSurface::pathCorrection(const GeometryContext& gctx,
                                       const Vector3& position,
                                       const Vector3& direction) const {
  Vector3 normalT = normal(gctx, position);
  double cosAlpha = normalT.dot(direction);
  return std::abs(1. / cosAlpha);
}

const CylinderBounds& CylinderSurface::bounds() const {
  return (*m_bounds.get());
}

Polyhedron CylinderSurface::polyhedronRepresentation(
    const GeometryContext& gctx, unsigned int quarterSegments) const {
  auto ctrans = transform(gctx);

  // Prepare vertices and faces
  std::vector<Vector3> vertices =
      bounds().circleVertices(ctrans, quarterSegments);
  auto [faces, triangularMesh] =
      detail::FacesHelper::cylindricalFaceMesh(vertices);
  return Polyhedron(vertices, faces, triangularMesh, false);
}

Vector3 CylinderSurface::rotSymmetryAxis(const GeometryContext& gctx) const {
  // fast access via transform matrix (and not rotation())
  return transform(gctx).matrix().block<3, 1>(0, 2);
}

detail::RealQuadraticEquation CylinderSurface::intersectionSolver(
    const Transform3& transform, const Vector3& position,
    const Vector3& direction) const {
  // Solve for radius R
  double R = bounds().get(CylinderBounds::eR);

  // Get the transformation matrtix
  const auto& tMatrix = transform.matrix();
  Vector3 caxis = tMatrix.block<3, 1>(0, 2).transpose();
  Vector3 ccenter = tMatrix.block<3, 1>(0, 3).transpose();

  // Check documentation for explanation
  Vector3 pc = position - ccenter;
  Vector3 pcXcd = pc.cross(caxis);
  Vector3 ldXcd = direction.cross(caxis);
  double a = ldXcd.dot(ldXcd);
  double b = 2. * (ldXcd.dot(pcXcd));
  double c = pcXcd.dot(pcXcd) - (R * R);
  // And solve the qaudratic equation
  return detail::RealQuadraticEquation(a, b, c);
}

SurfaceMultiIntersection CylinderSurface::intersect(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, const BoundaryTolerance& boundaryTolerance,
    double tolerance) const {
  const auto& gctxTransform = transform(gctx);

  // Solve the quadratic equation
  auto qe = intersectionSolver(gctxTransform, position, direction);

  // If no valid solution return a non-valid surfaceIntersection
  if (qe.solutions == 0) {
    return {{Intersection3D::invalid(), Intersection3D::invalid()}, this};
  }

  // Check the validity of the first solution
  Vector3 solution1 = position + qe.first * direction;
  IntersectionStatus status1 = std::abs(qe.first) < std::abs(tolerance)
                                   ? IntersectionStatus::onSurface
                                   : IntersectionStatus::reachable;

  // Helper method for boundary check
  auto boundaryCheck = [&](const Vector3& solution,
                           IntersectionStatus status) -> IntersectionStatus {
    // No check to be done, return current status
    if (boundaryTolerance.isInfinite()) {
      return status;
    }
    const auto& cBounds = bounds();
    if (auto absoluteBound = boundaryTolerance.asAbsoluteBoundOpt();
        absoluteBound.has_value() && cBounds.coversFullAzimuth()) {
      // Project out the current Z value via local z axis
      // Built-in local to global for speed reasons
      const auto& tMatrix = gctxTransform.matrix();
      // Create the reference vector in local
      const Vector3 vecLocal(solution - tMatrix.block<3, 1>(0, 3));
      double cZ = vecLocal.dot(tMatrix.block<3, 1>(0, 2));
      double modifiedTolerance = tolerance + absoluteBound->tolerance1;
      double hZ = cBounds.get(CylinderBounds::eHalfLengthZ) + modifiedTolerance;
      return std::abs(cZ) < std::abs(hZ) ? status
                                         : IntersectionStatus::unreachable;
    }
    return isOnSurface(gctx, solution, direction, boundaryTolerance)
               ? status
               : IntersectionStatus::unreachable;
  };
  // Check first solution for boundary compatibility
  status1 = boundaryCheck(solution1, status1);
  // Set the intersection
  Intersection3D first(solution1, qe.first, status1);
  if (qe.solutions == 1) {
    return {{first, first}, this};
  }
  // Check the validity of the second solution
  Vector3 solution2 = position + qe.second * direction;
  IntersectionStatus status2 = std::abs(qe.second) < std::abs(tolerance)
                                   ? IntersectionStatus::onSurface
                                   : IntersectionStatus::reachable;
  // Check first solution for boundary compatibility
  status2 = boundaryCheck(solution2, status2);
  Intersection3D second(solution2, qe.second, status2);
  // Order based on path length
  if (first.pathLength() <= second.pathLength()) {
    return {{first, second}, this};
  }
  return {{second, first}, this};
}

AlignmentToPathMatrix CylinderSurface::alignmentToPathDerivative(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  assert(isOnSurface(gctx, position, direction, BoundaryTolerance::Infinite()));

  // The vector between position and center
  const auto pcRowVec = (position - center(gctx)).transpose().eval();
  // The rotation
  const auto& rotation = transform(gctx).rotation();
  // The local frame x/y/z axis
  const auto& localXAxis = rotation.col(0);
  const auto& localYAxis = rotation.col(1);
  const auto& localZAxis = rotation.col(2);
  // The local coordinates
  const auto localPos = (rotation.transpose() * position).eval();
  const auto dx = direction.dot(localXAxis);
  const auto dy = direction.dot(localYAxis);
  const auto dz = direction.dot(localZAxis);
  // The normalization factor
  const auto norm = 1 / (1 - dz * dz);
  // The direction transpose
  const auto& dirRowVec = direction.transpose();
  // The derivative of path w.r.t. the local axes
  // @note The following calculations assume that the intersection of the track
  // with the cylinder always satisfy: perp(localPos) = R
  const auto localXAxisToPath =
      (-2 * norm * (dx * pcRowVec + localPos.x() * dirRowVec)).eval();
  const auto localYAxisToPath =
      (-2 * norm * (dy * pcRowVec + localPos.y() * dirRowVec)).eval();
  const auto localZAxisToPath =
      (-4 * norm * norm * (dx * localPos.x() + dy * localPos.y()) * dz *
       dirRowVec)
          .eval();
  // Calculate the derivative of local frame axes w.r.t its rotation
  const auto [rotToLocalXAxis, rotToLocalYAxis, rotToLocalZAxis] =
      detail::rotationToLocalAxesDerivative(rotation);
  // Initialize the derivative of propagation path w.r.t. local frame
  // translation (origin) and rotation
  AlignmentToPathMatrix alignToPath = AlignmentToPathMatrix::Zero();
  alignToPath.segment<3>(eAlignmentCenter0) =
      2 * norm * (dx * localXAxis.transpose() + dy * localYAxis.transpose());
  alignToPath.segment<3>(eAlignmentRotation0) =
      localXAxisToPath * rotToLocalXAxis + localYAxisToPath * rotToLocalYAxis +
      localZAxisToPath * rotToLocalZAxis;

  return alignToPath;
}

ActsMatrix<2, 3> CylinderSurface::localCartesianToBoundLocalDerivative(
    const GeometryContext& gctx, const Vector3& position) const {
  using VectorHelpers::perp;
  using VectorHelpers::phi;
  // The local frame transform
  const auto& sTransform = transform(gctx);
  // calculate the transformation to local coordinates
  const Vector3 localPos = sTransform.inverse() * position;
  const double lr = perp(localPos);
  const double lphi = phi(localPos);
  const double lcphi = std::cos(lphi);
  const double lsphi = std::sin(lphi);
  // Solve for radius R
  double R = bounds().get(CylinderBounds::eR);
  ActsMatrix<2, 3> loc3DToLocBound = ActsMatrix<2, 3>::Zero();
  loc3DToLocBound << -R * lsphi / lr, R * lcphi / lr, 0, 0, 0, 1;

  return loc3DToLocBound;
}

std::pair<std::shared_ptr<CylinderSurface>, bool> CylinderSurface::mergedWith(
    const CylinderSurface& other, AxisDirection direction,
    bool externalRotation, const Logger& logger) const {
  using namespace UnitLiterals;

  ACTS_VERBOSE("Merging cylinder surfaces in " << axisDirectionName(direction)
                                               << " direction");

  if (m_associatedDetElement != nullptr ||
      other.m_associatedDetElement != nullptr) {
    throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                  "CylinderSurface::merge: surfaces are "
                                  "associated with a detector element");
  }

  assert(m_transform != nullptr && other.m_transform != nullptr);

  Transform3 otherLocal = m_transform->inverse() * *other.m_transform;

  constexpr auto tolerance = s_onSurfaceTolerance;

  // surface cannot have any relative rotation

  if (std::abs(otherLocal.linear().col(eX)[eZ]) >= tolerance ||
      std::abs(otherLocal.linear().col(eY)[eZ]) >= tolerance) {
    ACTS_ERROR("CylinderSurface::merge: surfaces have relative rotation");
    throw SurfaceMergingException(
        getSharedPtr(), other.getSharedPtr(),
        "CylinderSurface::merge: surfaces have relative rotation");
  }

  auto checkNoBevel = [this, &logger, &other](const auto& bounds) {
    if (bounds.get(CylinderBounds::eBevelMinZ) != 0.0) {
      ACTS_ERROR(
          "CylinderVolumeStack requires all volumes to have a bevel angle of "
          "0");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "CylinderVolumeStack requires all volumes to have a bevel angle of "
          "0");
    }

    if (bounds.get(CylinderBounds::eBevelMaxZ) != 0.0) {
      ACTS_ERROR(
          "CylinderVolumeStack requires all volumes to have a bevel angle of "
          "0");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "CylinderVolumeStack requires all volumes to have a bevel angle of "
          "0");
    }
  };

  checkNoBevel(bounds());
  checkNoBevel(other.bounds());

  // radii need to be identical
  if (std::abs(bounds().get(CylinderBounds::eR) -
               other.bounds().get(CylinderBounds::eR)) > tolerance) {
    ACTS_ERROR("CylinderSurface::merge: surfaces have different radii");
    throw SurfaceMergingException(
        getSharedPtr(), other.getSharedPtr(),
        "CylinderSurface::merge: surfaces have different radii");
  }

  double r = bounds().get(CylinderBounds::eR);

  // no translation in x/z is allowed
  Vector3 translation = otherLocal.translation();

  if (std::abs(translation[0]) > tolerance ||
      std::abs(translation[1]) > tolerance) {
    ACTS_ERROR(
        "CylinderSurface::merge: surfaces have relative translation in x/y");
    throw SurfaceMergingException(
        getSharedPtr(), other.getSharedPtr(),
        "CylinderSurface::merge: surfaces have relative translation in x/y");
  }

  double hlZ = bounds().get(CylinderBounds::eHalfLengthZ);
  double minZ = -hlZ;
  double maxZ = hlZ;

  double zShift = translation[2];
  double otherHlZ = other.bounds().get(CylinderBounds::eHalfLengthZ);
  double otherMinZ = -otherHlZ + zShift;
  double otherMaxZ = otherHlZ + zShift;

  double hlPhi = bounds().get(CylinderBounds::eHalfPhiSector);
  double avgPhi = bounds().get(CylinderBounds::eAveragePhi);

  double otherHlPhi = other.bounds().get(CylinderBounds::eHalfPhiSector);
  double otherAvgPhi = other.bounds().get(CylinderBounds::eAveragePhi);

  if (direction == AxisDirection::AxisZ) {
    // z shift must match the bounds

    if (std::abs(otherLocal.linear().col(eY)[eX]) >= tolerance &&
        (!bounds().coversFullAzimuth() ||
         !other.bounds().coversFullAzimuth())) {
      throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                    "CylinderSurface::merge: surfaces have "
                                    "relative rotation in z and phi sector");
    }

    ACTS_VERBOSE("this: [" << minZ << ", " << maxZ << "] ~> "
                           << (minZ + maxZ) / 2.0 << " +- " << hlZ);
    ACTS_VERBOSE("zShift: " << zShift);

    ACTS_VERBOSE("other: [" << otherMinZ << ", " << otherMaxZ << "] ~> "
                            << (otherMinZ + otherMaxZ) / 2.0 << " +- "
                            << otherHlZ);
    if (std::abs(maxZ - otherMinZ) > tolerance &&
        std::abs(minZ - otherMaxZ) > tolerance) {
      ACTS_ERROR("CylinderSurface::merge: surfaces have incompatible z bounds");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "CylinderSurface::merge: surfaces have incompatible z bounds");
    }

    if (hlPhi != otherHlPhi || avgPhi != otherAvgPhi) {
      throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                    "CylinderSurface::merge: surfaces have "
                                    "different phi sectors");
    }

    double newMaxZ = std::max(maxZ, otherMaxZ);
    double newMinZ = std::min(minZ, otherMinZ);
    double newHlZ = (newMaxZ - newMinZ) / 2.0;
    double newMidZ = (newMaxZ + newMinZ) / 2.0;
    ACTS_VERBOSE("merged: [" << newMinZ << ", " << newMaxZ << "] ~> " << newMidZ
                             << " +- " << newHlZ);

    auto newBounds = std::make_shared<CylinderBounds>(r, newHlZ, hlPhi, avgPhi);

    Transform3 newTransform =
        *m_transform * Translation3{Vector3::UnitZ() * newMidZ};

    return {Surface::makeShared<CylinderSurface>(newTransform, newBounds),
            zShift < 0};

  } else if (direction == AxisDirection::AxisRPhi) {
    // no z shift is allowed
    if (std::abs(translation[2]) > tolerance) {
      ACTS_ERROR(
          "CylinderSurface::merge: surfaces have relative translation in z for "
          "rPhi merging");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "CylinderSurface::merge: surfaces have relative translation in z for "
          "rPhi merging");
    }

    if (hlZ != otherHlZ) {
      throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                    "CylinderSurface::merge: surfaces have "
                                    "different z bounds");
    }

    // Figure out signed relative rotation
    Vector2 rotatedX = otherLocal.linear().col(eX).head<2>();
    double zrotation = std::atan2(rotatedX[1], rotatedX[0]);

    ACTS_VERBOSE("this:  [" << avgPhi / 1_degree << " +- " << hlPhi / 1_degree
                            << "]");
    ACTS_VERBOSE("other: [" << otherAvgPhi / 1_degree << " +- "
                            << otherHlPhi / 1_degree << "]");

    ACTS_VERBOSE("Relative rotation around local z: " << zrotation / 1_degree);

    double prevOtherAvgPhi = otherAvgPhi;
    otherAvgPhi = detail::radian_sym(otherAvgPhi + zrotation);
    ACTS_VERBOSE("~> local other average phi: "
                 << otherAvgPhi / 1_degree
                 << " (was: " << prevOtherAvgPhi / 1_degree << ")");

    try {
      auto [newHlPhi, newAvgPhi, reversed] = detail::mergedPhiSector(
          hlPhi, avgPhi, otherHlPhi, otherAvgPhi, logger, tolerance);

      Transform3 newTransform = *m_transform;

      if (externalRotation) {
        ACTS_VERBOSE("Modifying transform for external rotation of "
                     << newAvgPhi / 1_degree);
        newTransform = newTransform * AngleAxis3(newAvgPhi, Vector3::UnitZ());
        newAvgPhi = 0.;
      }

      auto newBounds = std::make_shared<CylinderBounds>(
          r, bounds().get(CylinderBounds::eHalfLengthZ), newHlPhi, newAvgPhi);

      return {Surface::makeShared<CylinderSurface>(newTransform, newBounds),
              reversed};
    } catch (const std::invalid_argument& e) {
      throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                    e.what());
    }
  } else {
    throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                  "CylinderSurface::merge: invalid direction " +
                                      axisDirectionName(direction));
  }
}

}  // namespace Acts
