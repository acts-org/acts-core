// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/StrawSurface.hpp"

#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/detail/FacesHelper.hpp"
#include "Acts/Surfaces/detail/VerticesHelper.hpp"

#include <numbers>
#include <utility>
#include <vector>

namespace Acts {

StrawSurface::StrawSurface(const Transform3& transform, double radius,
                           double halez)
    : GeometryObject(), LineSurface(transform, radius, halez) {}

StrawSurface::StrawSurface(const Transform3& transform,
                           std::shared_ptr<const LineBounds> lbounds)
    : GeometryObject(), LineSurface(transform, std::move(lbounds)) {}

StrawSurface::StrawSurface(const std::shared_ptr<const LineBounds>& lbounds,
                           const DetectorElementBase& detelement)
    : GeometryObject(), LineSurface(lbounds, detelement) {}

StrawSurface::StrawSurface(const StrawSurface& other)
    : GeometryObject(), LineSurface(other) {}

StrawSurface::StrawSurface(const GeometryContext& gctx,
                           const StrawSurface& other, const Transform3& shift)
    : GeometryObject(), LineSurface(gctx, other, shift) {}

StrawSurface& StrawSurface::operator=(const StrawSurface& other) {
  if (this != &other) {
    LineSurface::operator=(other);
    m_bounds = other.m_bounds;
  }
  return *this;
}

Polyhedron StrawSurface::polyhedronRepresentation(
    const GeometryContext& gctx, unsigned int quarterSegments) const {
  // Prepare vertices and faces
  std::vector<Vector3> vertices;
  std::vector<Polyhedron::FaceType> faces;
  std::vector<Polyhedron::FaceType> triangularMesh;

  const Transform3& ctransform = transform(gctx);
  // Draw the bounds if more than one segment are chosen
  if (quarterSegments > 0u) {
    double r = m_bounds->get(LineBounds::eR);
    // Write the two bows/circles on either side
    std::vector<int> sides = {-1, 1};
    for (auto& side : sides) {
      // Helper method to create the segment
      auto svertices = detail::VerticesHelper::segmentVertices(
          {r, r}, -std::numbers::pi, std::numbers::pi, {}, quarterSegments,
          Vector3(0., 0., side * m_bounds->get(LineBounds::eHalfLengthZ)),
          ctransform);
      vertices.insert(vertices.end(), svertices.begin(), svertices.end());
    }
    auto facesMesh = detail::FacesHelper::cylindricalFaceMesh(vertices);
    faces = facesMesh.first;
    triangularMesh = facesMesh.second;
  }

  std::size_t bvertices = vertices.size();
  Vector3 left(0, 0, -m_bounds->get(LineBounds::eHalfLengthZ));
  Vector3 right(0, 0, m_bounds->get(LineBounds::eHalfLengthZ));
  // The central wire/straw
  vertices.push_back(ctransform * left);
  vertices.push_back(ctransform * right);
  faces.push_back({bvertices, bvertices + 1});
  vertices.push_back(ctransform * Vector3(0., 0., 0.));
  triangularMesh.push_back({bvertices, bvertices + 2, bvertices + 1});

  return Polyhedron(vertices, faces, triangularMesh, false);
}

}  // namespace Acts
