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

#include <ostream>

namespace Acts {

/// Class representing a frustum shape. The frustum is defined using an origin,
/// a direction and an opening angle. These parameters are then used to
/// calculate a number of side planes, each having a position and a normal
/// vector. The "near plane" is assumed to coincide with the origin point, and
/// the normal with the "direction" of the frustum. No far plane is defined.
/// @tparam value_t The floating point value to use
/// @tparam DIM The number of dimensions of ambient space
/// @tparam SIDES The number of sides (= side planes) the frustum has (exactly 2
/// in 2D, minimum 3 in 3D)
template <typename value_t, std::size_t DIM, std::size_t SIDES>
class Frustum {
  using translation_t = Eigen::Translation<value_t, DIM>;

  static constexpr std::size_t n_normals = SIDES + 1;

 public:
  /// Re expose the value type
  using value_type = value_t;
  /// Vertex type based on the value type and dimension
  using VertexType = Eigen::Matrix<value_t, DIM, 1>;
  /// Vertex array type corresponding to the vertex type
  using vertex_array_type = Eigen::Array<value_t, DIM, 1>;
  /// Associated transform type
  using transform_type = Eigen::Transform<value_t, DIM, Eigen::Affine>;

  /// Re expose the number of dimensions
  static constexpr std::size_t dim = DIM;
  /// Re expose the number of sides
  static constexpr std::size_t sides = SIDES;

  /// Constructor for the 2D case.
  /// @param origin The origin of the frustum
  /// @param dir The direction of the frustum
  /// @param opening_angle The opening angle
  /// @note The @p opening_angle is defined as the angle between opposing side
  /// planes. The opening angle needs to be < pi.
  Frustum(const VertexType& origin, const VertexType& dir,
          value_type opening_angle)
    requires(DIM == 2);

  /// Constructor for the 3D case.
  /// @param origin The origin of the frustum
  /// @param dir The direction of the frustum
  /// @param opening_angle The opening angle
  /// @note The @p opening_angle is defined as the angle between opposing side
  /// planes. The opening angle needs to be < pi.
  Frustum(const VertexType& origin, const VertexType& dir,
          value_type opening_angle)
    requires(DIM == 3);

  /// Draw a representation of this frustum using a visualization helper
  /// @note This is only available for the 3D case.
  /// @param helper The visualization helper
  /// @param far_distance The distance to the virtual "far plane" at which point
  /// the side planes terminate visually.
  void draw(IVisualization3D& helper, value_type far_distance = 10) const
    requires(DIM == 3);

  /// Draw a representation of this frustum as an SVG string to an outstream
  /// @note This is only available for the 2D case.
  /// @param os The out stream to write to
  /// @param w The width of the output SVG
  /// @param h The height of the output SVG
  /// @param far_distance The distance to the virtual "far line" at which point
  /// the side lines terminate visually.
  /// @param unit Multiplicative factor to apply to internal distances
  std::ostream& svg(std::ostream& os, value_type w, value_type h,
                    value_type far_distance = 1, value_type unit = 20.) const
    requires(DIM == 2);

  /// Getter for the oriogin of the frustum
  /// @return The origin of the frustum
  const VertexType& origin() const { return m_origin; }

  /// Getter for the direction of the frustum
  /// @return The direction of the frustum
  const VertexType& dir() const { return m_normals[0]; }

  /// Getter for the normal vectors of the planes defining this frustum.
  /// @return Array containing the normal vectors for all planes.
  /// @note The size of the array that is returned is fixed to `number of sides + 1`
  const std::array<VertexType, SIDES + 1>& normals() const { return m_normals; }

  /// Transforms this frustum using a given transform and returns a new instance
  /// @param trf The transform to apply
  /// @return A copy of this frustum with the transform @p trf applied.
  Frustum<value_t, DIM, SIDES> transformed(const transform_type& trf) const;

 private:
  // private constructor with pre-calculated normals
  Frustum(const VertexType& origin, std::array<VertexType, SIDES + 1> normals)
      : m_origin(origin), m_normals(std::move(normals)) {}

  VertexType m_origin;
  // need one more for direction we're facing
  std::array<VertexType, SIDES + 1> m_normals;
};

}  // namespace Acts

#include "Acts/Utilities/Frustum.ipp"
