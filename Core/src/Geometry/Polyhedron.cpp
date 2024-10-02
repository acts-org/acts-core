// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/Polyhedron.hpp"

#include "Acts/Surfaces/detail/VerticesHelper.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

void Acts::Polyhedron::merge(const Acts::Polyhedron& other) {
  std::size_t cvert = vertices.size();
  vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());
  /// Add the new faces with offsets
  auto join = [&](std::vector<FaceType>& existing,
                  const std::vector<FaceType>& additional) -> void {
    for (const auto& aface : additional) {
      FaceType nface = aface;
      std::transform(nface.begin(), nface.end(), nface.begin(),
                     [&](std::size_t x) { return (x + cvert); });
      existing.push_back(nface);
    }
  };
  // For faces and triangular mesh
  join(faces, other.faces);
  join(triangularMesh, other.triangularMesh);
}

void Acts::Polyhedron::move(const Transform3& transform) {
  for_each(vertices.begin(), vertices.end(),
           [&](auto& v) { v = transform * v; });
}

Acts::Extent Acts::Polyhedron::extent(const Transform3& transform) const {
  Extent extent;
  auto vtxs = vertices;
  std::transform(vtxs.begin(), vtxs.end(), vtxs.begin(), [&](auto& v) {
    auto vt = (transform * v);
    extent.extend(vt);
    return (vt);
  });

  // Special checks of BinningValue::binR for hyper plane surfaces
  if (detail::VerticesHelper::onHyperPlane(vtxs)) {
    // Check inclusion of origin (i.e. convex around origin)
    Vector3 origin =
        transform * Vector3(0., 0., extent.medium(BinningValue::binZ));
    for (const auto& face : faces) {
      std::vector<Vector3> tface;
      tface.reserve(face.size());
      for (auto f : face) {
        tface.push_back(vtxs[f]);
      }
      if (detail::VerticesHelper::isInsidePolygon(origin, tface)) {
        extent.range(BinningValue::binR).setMin(0.);
        extent.range(BinningValue::binPhi).set(-M_PI, M_PI);
        break;
      }
    }
    if (exact) {
      // Check for radial extend in 2D
      auto radialDistance = [&](const Vector3& pos1,
                                const Vector3& pos2) -> double {
        Vector2 O(0, 0);
        Vector2 p1p2 = (pos2.block<2, 1>(0, 0) - pos1.block<2, 1>(0, 0));
        double L = p1p2.norm();
        Vector2 p1O = (O - pos1.block<2, 1>(0, 0));

        // Don't try parallel lines
        if (L < 1e-7) {
          return std::numeric_limits<double>::max();
        }
        double f = p1p2.dot(p1O) / L;

        // Clamp to [0, |p1p2|]
        f = std::min(L, std::max(0., f));
        Vector2 closest = f * p1p2.normalized() + pos1.block<2, 1>(0, 0);
        double dist = (closest - O).norm();
        return dist;
      };

      for (std::size_t iv = 1; iv < vtxs.size() + 1; ++iv) {
        std::size_t fpoint = iv < vtxs.size() ? iv : 0;
        double testR = radialDistance(vtxs[fpoint], vtxs[iv - 1]);
        extent.range(BinningValue::binR).expandMin(testR);
      }
    }
  }
  return extent;
}

void Acts::Polyhedron::visualize(IVisualization3D& helper,
                                 const ViewConfig& viewConfig) const {
  if (viewConfig.visible) {
    if (!viewConfig.triangulate) {
      helper.faces(vertices, faces, viewConfig.color);
    } else {
      helper.faces(vertices, triangularMesh, viewConfig.color);
    }
  }
}
