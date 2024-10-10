// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Polyhedron.hpp"

#include <numeric>
#include <utility>
#include <vector>

namespace Acts::detail {

/// @brief Helper for writing out faces for polyhedron representation
struct FacesHelper {
  using FaceVector = std::vector<Polyhedron::FaceType>;

  /// @brief This method works for all convex type surface setups
  /// It includes:
  ///
  /// Rectangle / Triangle / Polygon
  /// Full disc, ellipse, half ring
  /// @param vertices The vector of vertices
  /// @param centerLast Boolean indicator if the center is given for
  /// a better triangulation method as last element of the vector
  static std::pair<FaceVector, FaceVector> convexFaceMesh(
      const std::vector<Vector3>& vertices, bool centerLast = false) {
    FaceVector faces;
    FaceVector triangularMesh;
    // Write the face
    unsigned int offset = centerLast ? 1 : 0;
    std::vector<std::size_t> face(vertices.size() - offset);
    std::iota(face.begin(), face.end(), 0);
    faces.push_back(face);
    /// Triangular mesh construction
    unsigned int anker = centerLast ? vertices.size() - 1 : 0;
    for (unsigned int it = 2 - offset; it < vertices.size() - offset; ++it) {
      triangularMesh.push_back({anker, it - 1, it});
    }
    // Close for centered reference point
    if (centerLast) {
      triangularMesh.push_back({anker, vertices.size() - 2, 0});
    }
    return {faces, triangularMesh};
  }

  /// @brief This method works for all concentric type surface setups
  /// It includes :
  ///
  /// - Cylinder (concentric bows on each side, separated by z)
  /// - Cut-off cone
  ///
  /// The single requirement is that the #vertices are equal and the input
  /// vector is splittable in half into the two bows.
  ///
  /// @param vertices The vector of vertices
  static std::pair<FaceVector, FaceVector> cylindricalFaceMesh(
      const std::vector<Vector3>& vertices) {
    FaceVector faces;
    FaceVector triangularMesh;
    std::size_t nqfaces = static_cast<std::size_t>(0.5 * vertices.size());
    for (std::size_t iface = 0; iface < nqfaces - 1; ++iface) {
      std::size_t p2 = (iface + 1 == nqfaces) ? 0 : iface + 1;
      std::vector<std::size_t> face = {iface, p2, p2 + nqfaces,
                                       nqfaces + iface};
      faces.push_back(face);
      std::vector<std::size_t> triA = {iface, p2, p2 + nqfaces};
      triangularMesh.push_back(triA);
      std::vector<std::size_t> triB = {p2 + nqfaces, nqfaces + iface, iface};
      triangularMesh.push_back(triB);
    }
    return {faces, triangularMesh};
  }
};

}  // namespace Acts::detail
