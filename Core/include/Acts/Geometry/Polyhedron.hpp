// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Visualization/ViewConfig.hpp"

#include <cstddef>
#include <vector>

namespace Acts {

class IVisualization3D;

/// @class Polyhedron
///
/// Struct which contains a cartesian approximation for any surface type.
/// It contains a list of cartesian vertices in the global frame, and
/// additionally
/// a list of lists of indices which indicate which vertices form a face.
/// Each entry in @c faces is a face, which is in turn a list of vertices
/// that need to be connected to form a face.
/// This allows the @c objString method to produce a ready-to-go obj output.
struct Polyhedron {
  using FaceType = std::vector<std::size_t>;

  /// Default constructor
  Polyhedron() = default;

  /// Default constructor from a vector of vertices and a vector of faces
  /// @param verticesIn The 3D global vertices that make up the object
  /// @param facesIn List of lists of indices for faces.
  /// @param triangularMeshIn List of lists of indices for a triangular mesh
  /// @param isExact A dedicated flag if this is exact or not
  ///
  /// @note This creates copies of the input vectors
  Polyhedron(const std::vector<Vector3>& verticesIn,
             const std::vector<FaceType>& facesIn,
             const std::vector<FaceType>& triangularMeshIn, bool isExact = true)
      : vertices(verticesIn),
        faces(facesIn),
        triangularMesh(triangularMeshIn),
        exact(isExact) {}

  /// List of 3D vertices as vectors
  std::vector<Vector3> vertices;

  /// List of faces connecting the vertices.
  /// each face is a list of vertices v
  /// corresponding to the vertex vector above
  std::vector<FaceType> faces;

  /// List of faces connecting the vertices.
  /// each face is a list of vertices v
  /// - in this case restricted to a triangular representation
  std::vector<FaceType> triangularMesh;

  /// Is this an exact representation (approximating curved spaces)
  bool exact = true;

  /// Merge another Polyhedron into this one
  ///
  /// @param other is the source representation
  void merge(const Polyhedron& other);

  /// Move the polyhedron with a Transfrom3D
  ///
  /// @param transform The additional transform applied
  void move(const Transform3& transform);

  /// Maximum extent of the polyhedron in space
  ///
  /// @param transform An (optional) transform
  /// to apply to the vertices for estimation the extent
  /// with respect to a given coordinate frame
  ///
  /// @return ranges that describe the space taken by this surface
  Extent extent(const Transform3& transform = Transform3::Identity()) const;

  void visualize(IVisualization3D& helper,
                 const ViewConfig& viewConfig = {}) const;
};
}  // namespace Acts
