// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/GenericCuboidVolumeBounds.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

Acts::GenericCuboidVolumeBounds::GenericCuboidVolumeBounds(
    const std::array<Acts::Vector3, 8>& vertices) noexcept(false)
    : m_vertices(vertices) {
  construct();
}

Acts::GenericCuboidVolumeBounds::GenericCuboidVolumeBounds(
    const std::array<double, GenericCuboidVolumeBounds::BoundValues::eSize>&
        values) noexcept(false)
    : m_vertices() {
  for (std::size_t iv = 0; iv < 8; ++iv) {
    m_vertices[iv] =
        Vector3(values[iv * 3], values[iv * 3 + 1], values[iv * 3 + 2]);
  }
  construct();
}

bool Acts::GenericCuboidVolumeBounds::inside(const Acts::Vector3& gpos,
                                             double tol) const {
  constexpr std::array<std::size_t, 6> vtxs = {0, 4, 0, 1, 2, 1};
  // needs to be on same side, get ref
  bool ref = std::signbit((gpos - m_vertices[vtxs[0]]).dot(m_normals[0]));
  for (std::size_t i = 1; i < 6; i++) {
    double dot = (gpos - m_vertices[vtxs[i]]).dot(m_normals[i]);
    if (std::signbit(dot) != ref) {
      // technically outside, but how far?
      if (std::abs(dot) > tol) {
        // distance greater than tol
        return false;
      }
      // distance smaller than tol, ignore
    }
  }
  return true;
}

std::vector<Acts::OrientedSurface>
Acts::GenericCuboidVolumeBounds::orientedSurfaces(
    const Transform3& transform) const {
  std::vector<OrientedSurface> oSurfaces;

  // approximate cog of the volume
  Vector3 cog(0, 0, 0);

  for (std::size_t i = 0; i < 8; i++) {
    cog += m_vertices[i];
  }

  cog *= 0.125;  // 1/8.

  auto make_surface = [&](const auto& a, const auto& b, const auto& c,
                          const auto& d) -> void {
    // calculate centroid of these points
    Vector3 ctrd = (a + b + c + d) / 4.;
    // create normal
    const Vector3 ab = b - a, ac = c - a;
    Vector3 normal = ab.cross(ac).normalized();

    Direction dir = Direction::fromScalar((cog - d).dot(normal));

    // build transform from z unit to normal
    // z is normal in local coordinates
    // Volume local to surface local
    Transform3 vol2srf;

    // GCC13+ Complains about maybe uninitialized memory inside Eigen's SVD code
    // This warning is ignored in this compilation unit by using the pragma at
    // the top of this file.
    vol2srf = (Eigen::Quaternion<Transform3::Scalar>().setFromTwoVectors(
        normal, Vector3::UnitZ()));

    vol2srf = vol2srf * Translation3(-ctrd);

    // now calculate position of vertices in surface local frame
    Vector3 a_l, b_l, c_l, d_l;
    a_l = vol2srf * a;
    b_l = vol2srf * b;
    c_l = vol2srf * c;
    d_l = vol2srf * d;

    std::vector<Vector2> vertices({{a_l.x(), a_l.y()},
                                   {b_l.x(), b_l.y()},
                                   {c_l.x(), c_l.y()},
                                   {d_l.x(), d_l.y()}});

    auto polyBounds = std::make_shared<const ConvexPolygonBounds<4>>(vertices);
    auto srfTrf = transform * vol2srf.inverse();
    auto srf = Surface::makeShared<PlaneSurface>(srfTrf, polyBounds);

    oSurfaces.push_back(OrientedSurface{std::move(srf), dir});
  };

  make_surface(m_vertices[0], m_vertices[1], m_vertices[2], m_vertices[3]);
  make_surface(m_vertices[4], m_vertices[5], m_vertices[6], m_vertices[7]);
  make_surface(m_vertices[0], m_vertices[3], m_vertices[7], m_vertices[4]);
  make_surface(m_vertices[1], m_vertices[2], m_vertices[6], m_vertices[5]);
  make_surface(m_vertices[2], m_vertices[3], m_vertices[7], m_vertices[6]);
  make_surface(m_vertices[1], m_vertices[0], m_vertices[4], m_vertices[5]);

  return oSurfaces;
}

std::ostream& Acts::GenericCuboidVolumeBounds::toStream(
    std::ostream& sl) const {
  sl << "Acts::GenericCuboidVolumeBounds: vertices (x, y, z) =\n";
  for (std::size_t i = 0; i < 8; i++) {
    if (i > 0) {
      sl << ",\n";
    }
    sl << "[" << m_vertices[i].transpose() << "]";
  }
  return sl;
}

void Acts::GenericCuboidVolumeBounds::construct() noexcept(false) {
  // calculate approximate center of gravity first, so we can make sure
  // the normals point inwards
  Vector3 cog(0, 0, 0);

  for (std::size_t i = 0; i < 8; i++) {
    cog += m_vertices[i];
  }

  cog *= 0.125;  // 1/8.

  std::size_t idx = 0;

  auto handle_face = [&](const auto& a, const auto& b, const auto& c,
                         const auto& d) {
    // we assume a b c d to be counter clockwise
    const Vector3 ab = b - a, ac = c - a;
    Vector3 normal = ab.cross(ac).normalized();

    if ((cog - a).dot(normal) < 0) {
      // normal points outwards, flip normal
      normal *= -1.;
    }

    // get rid of -0 values if present
    normal += Vector3::Zero();

    // check if d is on the surface
    if (std::abs((a - d).dot(normal)) > 1e-6) {
      throw(std::invalid_argument(
          "GenericCuboidBounds: Four points do not lie on the same plane!"));
    }

    m_normals[idx] = normal;
    idx++;
  };

  // handle faces
  handle_face(m_vertices[0], m_vertices[1], m_vertices[2], m_vertices[3]);
  handle_face(m_vertices[4], m_vertices[5], m_vertices[6], m_vertices[7]);
  handle_face(m_vertices[0], m_vertices[3], m_vertices[7], m_vertices[4]);
  handle_face(m_vertices[1], m_vertices[2], m_vertices[6], m_vertices[5]);
  handle_face(m_vertices[2], m_vertices[3], m_vertices[7], m_vertices[6]);
  handle_face(m_vertices[1], m_vertices[0], m_vertices[4], m_vertices[5]);
}

std::vector<double> Acts::GenericCuboidVolumeBounds::values() const {
  std::vector<double> rvalues;
  rvalues.reserve(BoundValues::eSize);
  for (std::size_t iv = 0; iv < 8; ++iv) {
    for (std::size_t ic = 0; ic < 3; ++ic) {
      rvalues.push_back(m_vertices[iv][ic]);
    }
  }
  return rvalues;
}

Acts::Volume::BoundingBox Acts::GenericCuboidVolumeBounds::boundingBox(
    const Acts::Transform3* trf, const Vector3& envelope,
    const Volume* entity) const {
  Vector3 vmin, vmax;

  Transform3 transform = Transform3::Identity();
  if (trf != nullptr) {
    transform = *trf;
  }

  vmin = transform * m_vertices[0];
  vmax = transform * m_vertices[0];

  for (std::size_t i = 1; i < 8; i++) {
    Vector3 vtx = transform * m_vertices[i];
    vmin = vmin.cwiseMin(vtx);
    vmax = vmax.cwiseMax(vtx);
  }

  return {entity, vmin - envelope, vmax + envelope};
}

void Acts::GenericCuboidVolumeBounds::draw(IVisualization3D& helper,
                                           const Transform3& transform) const {
  auto draw_face = [&](const auto& a, const auto& b, const auto& c,
                       const auto& d) {
    helper.face(std::vector<Vector3>(
        {transform * a, transform * b, transform * c, transform * d}));
  };

  draw_face(m_vertices[0], m_vertices[1], m_vertices[2], m_vertices[3]);
  draw_face(m_vertices[4], m_vertices[5], m_vertices[6], m_vertices[7]);
  draw_face(m_vertices[0], m_vertices[3], m_vertices[7], m_vertices[4]);
  draw_face(m_vertices[1], m_vertices[2], m_vertices[6], m_vertices[5]);
  draw_face(m_vertices[2], m_vertices[3], m_vertices[7], m_vertices[6]);
  draw_face(m_vertices[1], m_vertices[0], m_vertices[4], m_vertices[5]);
}
