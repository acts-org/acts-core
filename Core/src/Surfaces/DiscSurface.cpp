// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/DiscSurface.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/InfiniteBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/SurfaceError.hpp"
#include "Acts/Surfaces/SurfaceMergingException.hpp"
#include "Acts/Surfaces/detail/FacesHelper.hpp"
#include "Acts/Surfaces/detail/MergeHelper.hpp"
#include "Acts/Surfaces/detail/PlanarHelper.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/JacobianHelpers.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "Acts/Utilities/detail/periodic.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Acts {

using VectorHelpers::perp;
using VectorHelpers::phi;

DiscSurface::DiscSurface(const DiscSurface& other)
    : GeometryObject(), RegularSurface(other), m_bounds(other.m_bounds) {}

DiscSurface::DiscSurface(const GeometryContext& gctx, const DiscSurface& other,
                         const Transform3& shift)
    : GeometryObject(),
      RegularSurface(gctx, other, shift),
      m_bounds(other.m_bounds) {}

DiscSurface::DiscSurface(const Transform3& transform, double rmin, double rmax,
                         double hphisec)
    : GeometryObject(),
      RegularSurface(transform),
      m_bounds(std::make_shared<const RadialBounds>(rmin, rmax, hphisec)) {}

DiscSurface::DiscSurface(const Transform3& transform, double minhalfx,
                         double maxhalfx, double minR, double maxR,
                         double avephi, double stereo)
    : GeometryObject(),
      RegularSurface(transform),
      m_bounds(std::make_shared<const DiscTrapezoidBounds>(
          minhalfx, maxhalfx, minR, maxR, avephi, stereo)) {}

DiscSurface::DiscSurface(const Transform3& transform,
                         std::shared_ptr<const DiscBounds> dbounds)
    : GeometryObject(),
      RegularSurface(transform),
      m_bounds(std::move(dbounds)) {}

DiscSurface::DiscSurface(std::shared_ptr<const DiscBounds> dbounds,
                         const DetectorElementBase& detelement)
    : GeometryObject(),
      RegularSurface(detelement),
      m_bounds(std::move(dbounds)) {
  throw_assert(m_bounds, "nullptr as DiscBounds");
}

DiscSurface& DiscSurface::operator=(const DiscSurface& other) {
  if (this != &other) {
    Surface::operator=(other);
    m_bounds = other.m_bounds;
  }
  return *this;
}

Surface::SurfaceType DiscSurface::type() const {
  return Surface::Disc;
}

Vector3 DiscSurface::localToGlobal(const GeometryContext& gctx,
                                   const Vector2& lposition) const {
  // create the position in the local 3d frame
  Vector3 loc3Dframe(lposition[0] * cos(lposition[1]),
                     lposition[0] * sin(lposition[1]), 0.);
  // transform to globalframe
  return transform(gctx) * loc3Dframe;
}

Result<Vector2> DiscSurface::globalToLocal(const GeometryContext& gctx,
                                           const Vector3& position,
                                           double tolerance) const {
  // transport it to the globalframe
  Vector3 loc3Dframe = (transform(gctx).inverse()) * position;
  if (std::abs(loc3Dframe.z()) > std::abs(tolerance)) {
    return Result<Vector2>::failure(SurfaceError::GlobalPositionNotOnSurface);
  }
  return Result<Vector2>::success({perp(loc3Dframe), phi(loc3Dframe)});
}

Vector2 DiscSurface::localPolarToLocalCartesian(const Vector2& locpol) const {
  const DiscTrapezoidBounds* dtbo =
      dynamic_cast<const DiscTrapezoidBounds*>(&(bounds()));
  if (dtbo != nullptr) {
    double rMedium = dtbo->rCenter();
    double phi = dtbo->get(DiscTrapezoidBounds::eAveragePhi);

    Vector2 polarCenter(rMedium, phi);
    Vector2 cartCenter = localPolarToCartesian(polarCenter);
    Vector2 cartPos = localPolarToCartesian(locpol);
    Vector2 pos = cartPos - cartCenter;

    Vector2 locPos(pos[0] * sin(phi) - pos[1] * cos(phi),
                   pos[1] * sin(phi) + pos[0] * cos(phi));
    return Vector2(locPos[0], locPos[1]);
  }
  return Vector2(locpol[0] * cos(locpol[1]), locpol[0] * sin(locpol[1]));
}

Vector3 DiscSurface::localCartesianToGlobal(const GeometryContext& gctx,
                                            const Vector2& lposition) const {
  Vector3 loc3Dframe(lposition[0], lposition[1], 0.);
  return transform(gctx) * loc3Dframe;
}

Vector2 DiscSurface::globalToLocalCartesian(const GeometryContext& gctx,
                                            const Vector3& position,
                                            double /*direction*/) const {
  Vector3 loc3Dframe = (transform(gctx).inverse()) * position;
  return Vector2(loc3Dframe.x(), loc3Dframe.y());
}

std::string DiscSurface::name() const {
  return "Acts::DiscSurface";
}

const SurfaceBounds& DiscSurface::bounds() const {
  if (m_bounds) {
    return *m_bounds;
  }
  return s_noBounds;
}

Polyhedron DiscSurface::polyhedronRepresentation(
    const GeometryContext& gctx, unsigned int quarterSegments) const {
  // Prepare vertices and faces
  std::vector<Vector3> vertices;
  // Understand the disc
  bool fullDisc = m_bounds->coversFullAzimuth();
  bool toCenter = m_bounds->rMin() < s_onSurfaceTolerance;
  // If you have bounds you can create a polyhedron representation
  bool exactPolyhedron = (m_bounds->type() == SurfaceBounds::eDiscTrapezoid);
  bool addCentreFromConvexFace = (m_bounds->type() != SurfaceBounds::eAnnulus);
  if (m_bounds) {
    auto vertices2D = m_bounds->vertices(quarterSegments);
    vertices.reserve(vertices2D.size() + 1);
    Vector3 wCenter(0., 0., 0);
    for (const auto& v2D : vertices2D) {
      vertices.push_back(transform(gctx) * Vector3(v2D.x(), v2D.y(), 0.));
      wCenter += (*vertices.rbegin());
    }
    // These are convex shapes, use the helper method
    // For rings there's a sweet spot when this stops working
    if (exactPolyhedron || toCenter || !fullDisc) {
      // Transform them into the vertex frame
      wCenter *= 1. / vertices.size();
      if (addCentreFromConvexFace) {
        vertices.push_back(wCenter);
      }
      auto [faces, triangularMesh] = detail::FacesHelper::convexFaceMesh(
          vertices, addCentreFromConvexFace);
      return Polyhedron(vertices, faces, triangularMesh, exactPolyhedron);
    } else {
      // Two concentric rings, we use the pure concentric method momentarily,
      // but that creates too  many unneccesarry faces, when only two
      // are needed to describe the mesh, @todo investigate merging flag
      auto [faces, triangularMesh] =
          detail::FacesHelper::cylindricalFaceMesh(vertices);
      return Polyhedron(vertices, faces, triangularMesh, exactPolyhedron);
    }
  }
  throw std::domain_error("Polyhedron repr of boundless surface not possible.");
}

Vector2 DiscSurface::localPolarToCartesian(const Vector2& lpolar) const {
  return Vector2(lpolar[0] * cos(lpolar[1]), lpolar[0] * sin(lpolar[1]));
}

Vector2 DiscSurface::localCartesianToPolar(const Vector2& lcart) const {
  return Vector2(lcart.norm(), std::atan2(lcart[1], lcart[0]));
}

BoundToFreeMatrix DiscSurface::boundToFreeJacobian(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  assert(isOnSurface(gctx, position, direction, BoundaryTolerance::Infinite()));

  // The measurement frame of the surface
  RotationMatrix3 rframeT =
      referenceFrame(gctx, position, direction).transpose();

  // calculate the transformation to local coordinates
  const Vector3 posLoc = transform(gctx).inverse() * position;
  const double lr = perp(posLoc);
  const double lphi = phi(posLoc);
  const double lcphi = std::cos(lphi);
  const double lsphi = std::sin(lphi);
  // rotate into the polar coorindates
  auto lx = rframeT.block<1, 3>(0, 0);
  auto ly = rframeT.block<1, 3>(1, 0);
  // Initialize the jacobian from local to global
  BoundToFreeMatrix jacToGlobal = BoundToFreeMatrix::Zero();
  // the local error components - rotated from reference frame
  jacToGlobal.block<3, 1>(eFreePos0, eBoundLoc0) = lcphi * lx + lsphi * ly;
  jacToGlobal.block<3, 1>(eFreePos0, eBoundLoc1) =
      lr * (lcphi * ly - lsphi * lx);
  // the time component
  jacToGlobal(eFreeTime, eBoundTime) = 1;
  // the momentum components
  jacToGlobal.block<3, 2>(eFreeDir0, eBoundPhi) =
      sphericalToFreeDirectionJacobian(direction);
  jacToGlobal(eFreeQOverP, eBoundQOverP) = 1;
  return jacToGlobal;
}

FreeToBoundMatrix DiscSurface::freeToBoundJacobian(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction) const {
  using VectorHelpers::perp;
  using VectorHelpers::phi;

  assert(isOnSurface(gctx, position, direction, BoundaryTolerance::Infinite()));

  // The measurement frame of the surface
  RotationMatrix3 rframeT =
      referenceFrame(gctx, position, direction).transpose();

  // calculate the transformation to local coordinates
  const Vector3 posLoc = transform(gctx).inverse() * position;
  const double lr = perp(posLoc);
  const double lphi = phi(posLoc);
  const double lcphi = std::cos(lphi);
  const double lsphi = std::sin(lphi);
  // rotate into the polar coorindates
  auto lx = rframeT.block<1, 3>(0, 0);
  auto ly = rframeT.block<1, 3>(1, 0);
  // Initialize the jacobian from global to local
  FreeToBoundMatrix jacToLocal = FreeToBoundMatrix::Zero();
  // Local position component
  jacToLocal.block<1, 3>(eBoundLoc0, eFreePos0) = lcphi * lx + lsphi * ly;
  jacToLocal.block<1, 3>(eBoundLoc1, eFreePos0) =
      (lcphi * ly - lsphi * lx) / lr;
  // Time element
  jacToLocal(eBoundTime, eFreeTime) = 1;
  // Directional and momentum elements for reference frame surface
  jacToLocal.block<2, 3>(eBoundPhi, eFreeDir0) =
      freeToSphericalDirectionJacobian(direction);
  jacToLocal(eBoundQOverP, eFreeQOverP) = 1;
  return jacToLocal;
}

SurfaceMultiIntersection DiscSurface::intersect(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, const BoundaryTolerance& boundaryTolerance,
    double tolerance) const {
  // Get the contextual transform
  auto gctxTransform = transform(gctx);
  // Use the intersection helper for planar surfaces
  auto intersection =
      PlanarHelper::intersect(gctxTransform, position, direction, tolerance);
  auto status = intersection.status();
  // Evaluate boundary check if requested (and reachable)
  if (intersection.status() != IntersectionStatus::unreachable &&
      m_bounds != nullptr && !boundaryTolerance.isInfinite()) {
    // Built-in local to global for speed reasons
    const auto& tMatrix = gctxTransform.matrix();
    const Vector3 vecLocal(intersection.position() - tMatrix.block<3, 1>(0, 3));
    const Vector2 lcartesian = tMatrix.block<3, 2>(0, 0).transpose() * vecLocal;
    if (auto absoluteBound = boundaryTolerance.asAbsoluteBoundOpt();
        absoluteBound.has_value() && m_bounds->coversFullAzimuth()) {
      double modifiedTolerance = tolerance + absoluteBound->tolerance0;
      if (!m_bounds->insideRadialBounds(VectorHelpers::perp(lcartesian),
                                        modifiedTolerance)) {
        status = IntersectionStatus::unreachable;
      }
    } else if (!insideBounds(localCartesianToPolar(lcartesian),
                             boundaryTolerance)) {
      status = IntersectionStatus::unreachable;
    }
  }
  return {{Intersection3D(intersection.position(), intersection.pathLength(),
                          status),
           Intersection3D::invalid()},
          this};
}

ActsMatrix<2, 3> DiscSurface::localCartesianToBoundLocalDerivative(
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
  ActsMatrix<2, 3> loc3DToLocBound = ActsMatrix<2, 3>::Zero();
  loc3DToLocBound << lcphi, lsphi, 0, -lsphi / lr, lcphi / lr, 0;

  return loc3DToLocBound;
}

Vector3 DiscSurface::normal(const GeometryContext& gctx,
                            const Vector2& /*lposition*/) const {
  return normal(gctx);
}

Vector3 DiscSurface::normal(const GeometryContext& gctx,
                            const Vector3& /*position*/) const {
  return normal(gctx);
}

Vector3 DiscSurface::normal(const GeometryContext& gctx) const {
  // fast access via transform matrix (and not rotation())
  const auto& tMatrix = transform(gctx).matrix();
  return Vector3(tMatrix(0, 2), tMatrix(1, 2), tMatrix(2, 2));
}

Vector3 DiscSurface::referencePosition(const GeometryContext& gctx,
                                       AxisDirection aDir) const {
  if (aDir == AxisDirection::AxisR || aDir == AxisDirection::AxisPhi) {
    double r = m_bounds->binningValueR();
    double phi = m_bounds->binningValuePhi();
    return localToGlobal(gctx, Vector2{r, phi}, Vector3{});
  }
  return center(gctx);
}

double DiscSurface::referencePositionValue(const GeometryContext& gctx,
                                           AxisDirection aDir) const {
  if (aDir == AxisDirection::AxisR) {
    return VectorHelpers::perp(referencePosition(gctx, aDir));
  }
  if (aDir == AxisDirection::AxisPhi) {
    return VectorHelpers::phi(referencePosition(gctx, aDir));
  }

  return GeometryObject::referencePositionValue(gctx, aDir);
}

double DiscSurface::pathCorrection(const GeometryContext& gctx,
                                   const Vector3& /*position*/,
                                   const Vector3& direction) const {
  // we can ignore the global position here
  return 1. / std::abs(normal(gctx).dot(direction));
}

std::pair<std::shared_ptr<DiscSurface>, bool> DiscSurface::mergedWith(
    const DiscSurface& other, AxisDirection direction, bool externalRotation,
    const Logger& logger) const {
  using namespace UnitLiterals;

  ACTS_VERBOSE("Merging disc surfaces in " << direction << " direction");

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
    ACTS_ERROR("DiscSurface::merge: surfaces have relative rotation");
    throw SurfaceMergingException(
        getSharedPtr(), other.getSharedPtr(),
        "DiscSurface::merge: surfaces have relative rotation");
  }

  Vector3 translation = otherLocal.translation();

  if (std::abs(translation[0]) > tolerance ||
      std::abs(translation[1]) > tolerance ||
      std::abs(translation[2]) > tolerance) {
    ACTS_ERROR(
        "DiscSurface::merge: surfaces have relative translation in x/y/z");
    throw SurfaceMergingException(
        getSharedPtr(), other.getSharedPtr(),
        "DiscSurface::merge: surfaces have relative translation in x/y/z");
  }

  const auto* bounds = dynamic_cast<const RadialBounds*>(m_bounds.get());
  const auto* otherBounds =
      dynamic_cast<const RadialBounds*>(other.m_bounds.get());

  if (bounds == nullptr || otherBounds == nullptr) {
    ACTS_ERROR("DiscSurface::merge: surfaces have bounds other than radial");
    throw SurfaceMergingException(
        getSharedPtr(), other.getSharedPtr(),
        "DiscSurface::merge: surfaces have bounds other than radial");
  }

  double minR = bounds->get(RadialBounds::eMinR);
  double maxR = bounds->get(RadialBounds::eMaxR);

  double hlPhi = bounds->get(RadialBounds::eHalfPhiSector);
  double avgPhi = bounds->get(RadialBounds::eAveragePhi);
  double minPhi = detail::radian_sym(-hlPhi + avgPhi);
  double maxPhi = detail::radian_sym(hlPhi + avgPhi);

  ACTS_VERBOSE(" this: r =   [" << minR << ", " << maxR << "]");
  ACTS_VERBOSE("       phi = ["
               << minPhi / 1_degree << ", " << maxPhi / 1_degree << "] ~> "
               << avgPhi / 1_degree << " +- " << hlPhi / 1_degree);

  double otherMinR = otherBounds->get(RadialBounds::eMinR);
  double otherMaxR = otherBounds->get(RadialBounds::eMaxR);
  double otherAvgPhi = otherBounds->get(RadialBounds::eAveragePhi);
  double otherHlPhi = otherBounds->get(RadialBounds::eHalfPhiSector);
  double otherMinPhi = detail::radian_sym(-otherHlPhi + otherAvgPhi);
  double otherMaxPhi = detail::radian_sym(otherHlPhi + otherAvgPhi);

  ACTS_VERBOSE("other: r =   [" << otherMinR << ", " << otherMaxR << "]");
  ACTS_VERBOSE("       phi = [" << otherMinPhi / 1_degree << ", "
                                << otherMaxPhi / 1_degree << "] ~> "
                                << otherAvgPhi / 1_degree << " +- "
                                << otherHlPhi / 1_degree);

  if (direction == AxisDirection::AxisR) {
    if (std::abs(otherLocal.linear().col(eY)[eX]) >= tolerance &&
        (!bounds->coversFullAzimuth() || !otherBounds->coversFullAzimuth())) {
      throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                    "DiscSurface::merge: surfaces have "
                                    "relative rotation in z and phi sector");
    }

    if (std::abs(minR - otherMaxR) > tolerance &&
        std::abs(maxR - otherMinR) > tolerance) {
      ACTS_ERROR("DiscSurface::merge: surfaces are not touching r");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "DiscSurface::merge: surfaces are not touching in r");
    }

    if (std::abs(avgPhi - otherAvgPhi) > tolerance) {
      ACTS_ERROR("DiscSurface::merge: surfaces have different average phi");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "DiscSurface::merge: surfaces have different average phi");
    }

    if (std::abs(hlPhi - otherHlPhi) > tolerance) {
      ACTS_ERROR("DiscSurface::merge: surfaces have different half phi sector");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "DiscSurface::merge: surfaces have different half phi sector");
    }

    double newMinR = std::min(minR, otherMinR);
    double newMaxR = std::max(maxR, otherMaxR);
    ACTS_VERBOSE("  new: r =   [" << newMinR << ", " << newMaxR << "]");

    auto newBounds =
        std::make_shared<RadialBounds>(newMinR, newMaxR, hlPhi, avgPhi);

    return {Surface::makeShared<DiscSurface>(*m_transform, newBounds),
            minR > otherMinR};

  } else if (direction == AxisDirection::AxisPhi) {
    if (std::abs(maxR - otherMaxR) > tolerance ||
        std::abs(minR - otherMinR) > tolerance) {
      ACTS_ERROR("DiscSurface::merge: surfaces don't have same r bounds");
      throw SurfaceMergingException(
          getSharedPtr(), other.getSharedPtr(),
          "DiscSurface::merge: surfaces don't have same r bounds");
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

      auto newBounds =
          std::make_shared<RadialBounds>(minR, maxR, newHlPhi, newAvgPhi);

      return {Surface::makeShared<DiscSurface>(newTransform, newBounds),
              reversed};
    } catch (const std::invalid_argument& e) {
      throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                    e.what());
    }

  } else {
    ACTS_ERROR("DiscSurface::merge: invalid direction " << direction);

    throw SurfaceMergingException(getSharedPtr(), other.getSharedPtr(),
                                  "DiscSurface::merge: invalid direction " +
                                      axisDirectionName(direction));
  }
}

}  // namespace Acts
