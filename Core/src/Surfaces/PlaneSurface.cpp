// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/PlaneSurface.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/InfiniteBounds.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/SurfaceError.hpp"
#include "Acts/Surfaces/detail/FacesHelper.hpp"
#include "Acts/Surfaces/detail/PlanarHelper.hpp"
#include "Acts/Utilities/AlgebraHelpers.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

Acts::PlaneSurface::PlaneSurface(const PlaneSurface& other)
    : GeometryObject(), RegularSurface(other), m_bounds(other.m_bounds) {}

Acts::PlaneSurface::PlaneSurface(const GeometryContext& gctx,
                                 const PlaneSurface& other,
                                 const Transform3& transform)
    : GeometryObject(),
      RegularSurface(gctx, other, transform),
      m_bounds(other.m_bounds) {}

Acts::PlaneSurface::PlaneSurface(std::shared_ptr<const PlanarBounds> pbounds,
                                 const Acts::DetectorElementBase& detelement)
    : RegularSurface(detelement), m_bounds(std::move(pbounds)) {
  /// surfaces representing a detector element must have bounds
  throw_assert(m_bounds, "PlaneBounds must not be nullptr");
}

Acts::PlaneSurface::PlaneSurface(const Transform3& transform,
                                 std::shared_ptr<const PlanarBounds> pbounds)
    : RegularSurface(transform), m_bounds(std::move(pbounds)) {}

Acts::PlaneSurface& Acts::PlaneSurface::operator=(const PlaneSurface& other) {
  if (this != &other) {
    Surface::operator=(other);
    m_bounds = other.m_bounds;
  }
  return *this;
}

Acts::Surface::SurfaceType Acts::PlaneSurface::type() const {
  return Surface::Plane;
}

Acts::Vector3 Acts::PlaneSurface::localToGlobal(
    const GeometryContext& gctx, const Vector2& lposition) const {
  return transform(gctx) *
         Vector3(lposition[Acts::eBoundLoc0], lposition[Acts::eBoundLoc1], 0.);
}

Acts::Result<Acts::Vector2> Acts::PlaneSurface::globalToLocal(
    const GeometryContext& gctx, const Vector3& position,
    double tolerance) const {
  Vector3 loc3Dframe = inverseTransform(transform(gctx)) * position;
  if (std::abs(loc3Dframe.z()) > std::abs(tolerance)) {
    return Result<Vector2>::failure(SurfaceError::GlobalPositionNotOnSurface);
  }
  return Result<Vector2>::success({loc3Dframe.x(), loc3Dframe.y()});
}

std::string Acts::PlaneSurface::name() const {
  return "Acts::PlaneSurface";
}

const Acts::SurfaceBounds& Acts::PlaneSurface::bounds() const {
  if (m_bounds) {
    return (*m_bounds.get());
  }
  return s_noBounds;
}

Acts::Polyhedron Acts::PlaneSurface::polyhedronRepresentation(
    const GeometryContext& gctx, unsigned int quarterSegments) const {
  // Prepare vertices and faces
  std::vector<Vector3> vertices;
  bool exactPolyhedron = true;

  // If you have bounds you can create a polyhedron representation
  if (m_bounds) {
    auto vertices2D = m_bounds->vertices(quarterSegments);
    vertices.reserve(vertices2D.size() + 1);
    for (const auto& v2D : vertices2D) {
      vertices.push_back(transform(gctx) * Vector3(v2D.x(), v2D.y(), 0.));
    }
    bool isEllipse = bounds().type() == SurfaceBounds::eEllipse;
    bool innerExists = false, coversFull = false;
    if (isEllipse) {
      exactPolyhedron = false;
      auto vStore = bounds().values();
      innerExists = vStore[EllipseBounds::eInnerRx] > s_epsilon &&
                    vStore[EllipseBounds::eInnerRy] > s_epsilon;
      coversFull = std::abs(vStore[EllipseBounds::eHalfPhiSector] -
                            std::numbers::pi) < s_epsilon;
    }
    // All of those can be described as convex
    // @todo same as for Discs: coversFull is not the right criterium
    // for triangulation
    if (!isEllipse || !innerExists || !coversFull) {
      auto [faces, triangularMesh] =
          detail::FacesHelper::convexFaceMesh(vertices);
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
  throw std::domain_error(
      "Polyhedron representation of boundless surface not possible.");
}

Acts::Vector3 Acts::PlaneSurface::normal(const GeometryContext& gctx,
                                         const Vector2& /*lpos*/) const {
  return normal(gctx);
}

Acts::Vector3 Acts::PlaneSurface::normal(const GeometryContext& gctx,
                                         const Vector3& /*pos*/) const {
  return normal(gctx);
}

Acts::Vector3 Acts::PlaneSurface::normal(const GeometryContext& gctx) const {
  return transform(gctx).linear().col(2);
}

Acts::Vector3 Acts::PlaneSurface::binningPosition(
    const GeometryContext& gctx, BinningValue /*bValue*/) const {
  return center(gctx);
}

double Acts::PlaneSurface::pathCorrection(const GeometryContext& gctx,
                                          const Vector3& /*position*/,
                                          const Vector3& direction) const {
  // We can ignore the global position here
  return 1. / std::abs(normal(gctx).dot(direction));
}

Acts::SurfaceMultiIntersection Acts::PlaneSurface::intersect(
    const GeometryContext& gctx, const Vector3& position,
    const Vector3& direction, const BoundaryTolerance& boundaryTolerance,
    double tolerance) const {
  // Get the contextual transform
  const auto& gctxTransform = transform(gctx);
  // Use the intersection helper for planar surfaces
  auto intersection =
      PlanarHelper::intersect(gctxTransform, position, direction, tolerance);
  auto status = intersection.status();
  // Evaluate boundary check if requested (and reachable)
  if (intersection.status() != IntersectionStatus::unreachable) {
    // Built-in local to global for speed reasons
    const auto& tMatrix = gctxTransform.matrix();
    // Create the reference vector in local
    const Vector3 vecLocal(intersection.position() - tMatrix.block<3, 1>(0, 3));
    if (!insideBounds(tMatrix.block<3, 2>(0, 0).transpose() * vecLocal,
                      boundaryTolerance)) {
      status = IntersectionStatus::unreachable;
    }
  }
  return {{Intersection3D(intersection.position(), intersection.pathLength(),
                          status),
           Intersection3D::invalid()},
          this};
}

Acts::ActsMatrix<2, 3> Acts::PlaneSurface::localCartesianToBoundLocalDerivative(
    const GeometryContext& /*gctx*/, const Vector3& /*position*/) const {
  const ActsMatrix<2, 3> loc3DToLocBound = ActsMatrix<2, 3>::Identity();
  return loc3DToLocBound;
}
