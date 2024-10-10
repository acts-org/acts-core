// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Visualization/IVisualization3D.hpp"

namespace Acts {

/// Class which models a ray. It is defined by a starting point and a
/// direction.
/// @tparam value_t The floating point type to use
/// @tparam DIM The number of dimensions in which this ray is defined (2 or 3)
template <typename value_t, std::size_t DIM>
class Ray {
 public:
  /// Re expose the value type
  using value_type = value_t;
  /// Vertex type based on the value type and dimension
  using VertexType = Eigen::Matrix<value_t, DIM, 1>;
  /// Vertex array type corresponding to the vertex type
  using vertex_array_type = Eigen::Array<value_t, DIM, 1>;
  /// Associated transform type
  using transform_type = Eigen::Transform<value_t, DIM, Eigen::Affine>;

  /// Constructor from an origin point and a direction
  /// @param origin The origin of the ray
  /// @param dir The direction of the ray
  Ray(const VertexType& origin, const VertexType& dir);

  /// Getter for the origin
  /// @return The origin
  const VertexType& origin() const { return m_origin; }

  /// Getter for the direction
  /// @return The direction
  const VertexType& dir() const { return m_dir; }

  /// Getter for the element wise inverse of the direction.
  /// @return The element wise inverse.
  const vertex_array_type& idir() const { return m_idir; }

  /// Transforms this ray using a given transform and returns a new instance
  /// @param trf The transform to apply
  /// @return Copy of this ray with the transform applied
  Ray<value_t, DIM> transformed(const transform_type& trf) const;

  /// Write information on this instance to an outstream.
  /// @param os The out stream
  /// @return The out stream given as an argument
  std::ostream& toStream(std::ostream& os) const;

  /// Helper to draw this ray using a given visualization helper.
  /// @param helper The visualization helper
  /// @param far_distance The "length" of the drawn line representing the ray
  void draw(IVisualization3D& helper, value_type far_distance = 10) const
    requires(DIM == 3);

 private:
  VertexType m_origin;
  VertexType m_dir;
  vertex_array_type m_idir;
};

/// Overload of the outstream operator
/// @param os The out stream
/// @param ray The ray to write to @p os
/// @return The outstream given in @p os
template <typename T, std::size_t D>
std::ostream& operator<<(std::ostream& os, const Ray<T, D>& ray) {
  ray.dump(os);
  return os;
}

using Ray3D = Ray<ActsScalar, 3>;

}  // namespace Acts

#include "Acts/Utilities/Ray.ipp"
