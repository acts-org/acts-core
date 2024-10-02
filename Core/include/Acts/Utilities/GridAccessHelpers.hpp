// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <array>
#include <vector>

namespace Acts {

namespace GridAccessHelpers {

/// Unroll the cast loop
///
/// @tparam cast_container is the container type of cast objects
/// @tparam Array is the array type to be filled
///
/// @param position is the position of the update call
/// @param globalCasts is the cast value vector from global to grid position
/// @param ra is the array to be filled
template <typename cast_container, typename Array, std::size_t... idx>
void fillCasts(const Vector3& position, const cast_container& globalCasts,
               Array& ra, std::index_sequence<idx...> /*indices*/) {
  ((ra[idx] = VectorHelpers::cast(position, globalCasts[idx])), ...);
}
/// Cast into a lookup position
///
/// This method allows to transform a global position into a grid position
/// that is specified by binning values. It casts the global position info
/// into a format suitable for the grid.
///
/// @tparam cast_container is the container type of cast objects
/// @tparam Array is the array type to be filled
///
/// @param position is the position of the update call
/// @param globalCasts is the cast value vector from global to grid position
///
/// @return a grid point in an appropriate format
template <typename grid_type, typename cast_container>
typename grid_type::point_t castPosition(const Vector3& position,
                                         const cast_container& globalCasts) {
  // Fill the grid point from global
  typename grid_type::point_t casted{};
  fillCasts(position, globalCasts, casted,
            std::make_integer_sequence<std::size_t, grid_type::DIM>{});
  return casted;
}

/// Unroll the local position loop
///
/// @param lposition is the local position
/// @param laccess the local accessors
/// @param ra is the array to be filled
///
/// @note void function that fills the provided array
template <typename Array, typename local_indices, std::size_t... idx>
void fillLocal(const Vector2& lposition, const local_indices& laccess,
               Array& ra, std::index_sequence<idx...> /*indices*/) {
  ((ra[idx] = lposition[laccess[idx]]), ...);
}

/// Access local parameters for a propriate lookup position
///
/// This method allows to transform a local position into a
/// grid position, it only works for 1-D and 2-D grids.
///
/// @param lposition is the position of the update call
/// @param laccess the local accessors
///
/// @return an array suitable for the grid
template <typename grid_type, typename local_indices>
typename grid_type::point_t accessLocal(const Vector2& lposition,
                                        const local_indices& laccess) {
  if constexpr (grid_type::DIM > 2u) {
    throw std::invalid_argument(
        "GridAccessHelper: only 1-D and 2-D grids are possible for local "
        "access.");
  }
  // Fill the grid point from local according to the accessors
  typename grid_type::point_t accessed{};
  fillLocal(lposition, laccess, accessed,
            std::make_integer_sequence<std::size_t, grid_type::DIM>{});
  return accessed;
}

}  // namespace GridAccessHelpers

namespace GridAccess {

/// Interface class for owning delegate
class IGlobalToGridLocal {
 public:
  virtual ~IGlobalToGridLocal() = default;
};

/// Interface class for owning delegate
class IBoundToGridLocal {
 public:
  virtual ~IBoundToGridLocal() = default;
};

template <typename global_to_grid_local_t>
class Affine3Transformed final : public IGlobalToGridLocal {
 public:
  using grid_local_t = typename global_to_grid_local_t::grid_local_t;

  /// The global to local transformation
  global_to_grid_local_t globalToGridLocal;

  /// The transformation matrix
  Transform3 transform;

  /// Constructor
  ///
  /// @param g2gl is the global to grid local transformation
  /// @param t is the transformation matrix
  Affine3Transformed(global_to_grid_local_t g2gl, const Transform3& t)
      : globalToGridLocal(std::move(g2gl)), transform(t) {}

  /// Transform in to the local frame, then the grid local position
  ///
  /// @param position is the global position
  ///
  /// @return the grid position
  typename global_to_grid_local_t::grid_local_t toGridLocal(
      const Vector3& position) const {
    return globalToGridLocal.toGridLocal(transform * position);
  }
};

/// @brief A global (potentially casted) sub space of a global
/// position
/// @tparam ...Args
template <BinningValue... Args>
class GlobalSubspace final : public IGlobalToGridLocal {
 public:
  using grid_local_t = std::array<ActsScalar, sizeof...(Args)>;

  /// Assert that size has to be bigger than 0
  static_assert(sizeof...(Args) > 0,
                "GlobalSubspace: cannot have an empty binning value list.");

  /// Assert that size has to be smaller than 4
  static_assert(sizeof...(Args) <= 3,
                "GlobalSubspace: cannot have more than 3 binning values.");

  // Constructor
  GlobalSubspace() = default;

  /// The binning values
  static constexpr std::array<BinningValue, sizeof...(Args)> bValues = {
      Args...};

  /// Transform in to the local frame, then the grid local position
  ///
  /// @param position is the global position
  ///
  /// @return the grid position
  grid_local_t toGridLocal(const Vector3& position) const {
    // Fill the grid point from global
    grid_local_t glocal{};
    GridAccessHelpers::fillCasts(
        position, bValues, glocal,
        std::make_integer_sequence<std::size_t, sizeof...(Args)>{});
    return glocal;
  }
};

// The bound to grid local transformation, if only access of a subspace
// is requested
template <std::size_t... Args>
class LocalSubspace final : public IBoundToGridLocal {
 public:
  using grid_local_t = std::array<ActsScalar, sizeof...(Args)>;

  /// Assert that the accessors are unique
  static_assert(sizeof...(Args) == 1 || sizeof...(Args) == 2,
                "LocalSubspace: only 1 or 2 accessors are allowed.");

  /// Only 0 or 1 are allowed
  static_assert(((Args < 2) && ...),
                "LocalSubspace: local access needs to be 0u or 1u");

  // Constructor
  LocalSubspace() = default;

  static constexpr std::array<std::size_t, sizeof...(Args)> accessors = {
      Args...};

  /// Access the local entries
  ///
  /// @param lposition is the local position
  ///
  /// @return the grid position
  grid_local_t toGridLocal(const Vector2& lposition) const {
    // Fill the grid point from local according to the accessors
    grid_local_t accessed{};
    GridAccessHelpers::fillLocal(
        lposition, accessors, accessed,
        std::make_integer_sequence<std::size_t, sizeof...(Args)>{});
    return accessed;
  }
};

class BoundCylinderToZPhi final : public IBoundToGridLocal {
 public:
  ActsScalar radius = 1.;
  ActsScalar shift = 0.;

  /// Constructor with arguments
  /// @param r the radius
  /// @param z the shift
  BoundCylinderToZPhi(ActsScalar r, ActsScalar z) : radius(r), shift(z) {}

  std::array<ActsScalar, 2u> toGridLocal(const Vector2& local) const {
    return {local[1u] + shift, local[0u] / radius};
  }

  using BoundDiscToRPhi = LocalSubspace<0u, 1u>;
};

// Definition of bound (on surface) to grid local representation delegate
// 1 dimensional local grid
using BoundToGridLocal1DimDelegate =
    OwningDelegate<std::array<ActsScalar, 1u>(const Vector2&),
                   GridAccess::IBoundToGridLocal>;

// Definition of global to grid local representation delegate
// 1 dimensional local grid
using GlobalToGridLocal1DimDelegate =
    OwningDelegate<std::array<ActsScalar, 1u>(const Vector3&),
                   GridAccess::IGlobalToGridLocal>;

// Definition of bound (on surface) to grid local representation delegate
// 2 dimensional local grid
using BoundToGridLocal2DimDelegate =
    OwningDelegate<std::array<ActsScalar, 2u>(const Vector2&),
                   GridAccess::IBoundToGridLocal>;

// Definition of global to grid local representation delegate
// 2 dimensional local grid
using GlobalToGridLocal2DimDelegate =
    OwningDelegate<std::array<ActsScalar, 2u>(const Vector3&),
                   GridAccess::IGlobalToGridLocal>;

}  // namespace GridAccess
}  // namespace Acts
