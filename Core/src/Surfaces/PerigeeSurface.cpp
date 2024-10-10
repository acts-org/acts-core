// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Surfaces/PerigeeSurface.hpp"

#include "Acts/Geometry/GeometryObject.hpp"

#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

Acts::PerigeeSurface::PerigeeSurface(const Vector3& gp)
    : LineSurface(Transform3(Translation3(gp.x(), gp.y(), gp.z())), nullptr) {}

Acts::PerigeeSurface::PerigeeSurface(const Transform3& transform)
    : GeometryObject(), LineSurface(transform) {}

Acts::PerigeeSurface::PerigeeSurface(const PerigeeSurface& other)
    : GeometryObject(), LineSurface(other) {}

Acts::PerigeeSurface::PerigeeSurface(const GeometryContext& gctx,
                                     const PerigeeSurface& other,
                                     const Transform3& shift)
    : GeometryObject(), LineSurface(gctx, other, shift) {}

Acts::PerigeeSurface& Acts::PerigeeSurface::operator=(
    const PerigeeSurface& other) {
  if (this != &other) {
    LineSurface::operator=(other);
  }
  return *this;
}

Acts::Surface::SurfaceType Acts::PerigeeSurface::type() const {
  return Surface::Perigee;
}

std::string Acts::PerigeeSurface::name() const {
  return "Acts::PerigeeSurface";
}

std::ostream& Acts::PerigeeSurface::toStreamImpl(const GeometryContext& gctx,
                                                 std::ostream& sl) const {
  sl << std::setiosflags(std::ios::fixed);
  sl << std::setprecision(7);
  sl << "Acts::PerigeeSurface:" << std::endl;
  const Vector3& sfCenter = center(gctx);
  sl << "     Center position  (x, y, z) = (" << sfCenter.x() << ", "
     << sfCenter.y() << ", " << sfCenter.z() << ")";
  sl << std::setprecision(-1);
  return sl;
}

Acts::Polyhedron Acts::PerigeeSurface::polyhedronRepresentation(
    const GeometryContext& gctx, unsigned int /*quarterSegments*/) const {
  // Prepare vertices and faces
  std::vector<Vector3> vertices;
  std::vector<Polyhedron::FaceType> faces;
  std::vector<Polyhedron::FaceType> triangularMesh;

  const Transform3& ctransform = transform(gctx);
  Vector3 left(0, 0, -100.);
  Vector3 right(0, 0, 100.);

  // The central wire/straw
  vertices.push_back(ctransform * left);
  vertices.push_back(ctransform * right);
  faces.push_back({0, 1});
  vertices.push_back(ctransform * Vector3(0., 0., 0.));
  triangularMesh.push_back({0, 2, 1});

  return Polyhedron(vertices, faces, triangularMesh);
}
