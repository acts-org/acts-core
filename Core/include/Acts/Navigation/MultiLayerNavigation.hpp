// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationStateFillers.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <tuple>

namespace Acts::Experimental {

template <typename grid_t, typename path_generator>
class MultiLayerNavigation : public IInternalNavigation {
 public:
  /// Broadcast the grid type
  using grid_type = grid_t;

  /// The grid where the indices are stored
  grid_type grid;

  /// The path generator
  path_generator pgenerator;

  /// These are the cast parameters - copied from constructor
  std::array<BinningValue, grid_type::DIM> casts{};

  /// An inverse transform to be applied to the position
  Transform3 transform = Transform3::Identity();

  /// @brief  Constructor for a grid based surface attacher
  /// @param igrid the grid that is moved into this attacher
  /// @param icasts is the cast values array
  /// @param itr a transform applied to the global position
  MultiLayerNavigation(grid_type igrid,
                       const std::array<BinningValue, grid_type::DIM>& icasts,
                       const Transform3& itr = Transform3::Identity())
      : grid(std::move(igrid)), casts(icasts), transform(itr) {}

  MultiLayerNavigation() = delete;

  /// Fill the navigation state
  ///
  /// @note no initialization is done here (sorting and update)
  ///
  /// @param gctx is the geometry context
  /// @param nState is the navigation state
  void fill(const GeometryContext& gctx, NavigationState& nState) const {
    // get the local position and direction
    auto lposition = transform * nState.position;
    auto ldirection = transform.linear() * nState.direction;

    auto step = std::sqrt(std::pow(grid.binWidth()[0], 2) +
                          std::pow(grid.binWidth()[1], 2));
    auto path = pgenerator(lposition, ldirection, step, grid.numLocalBins()[1]);

    std::vector<const Acts::Surface*> surfCandidates = {};

    for (const auto& p : path) {
      const auto& entry = grid.atPosition(castPosition(p));
      const auto extracted =
          IndexedSurfacesExtractor::extract(gctx, nState, entry);
      surfCandidates.insert(surfCandidates.end(), extracted.begin(),
                            extracted.end());
    }

    resolveDuplicates(gctx, surfCandidates);
    SurfacesFiller::fill(nState, surfCandidates);
  }
  /// Fill the update the navigation state with candidates
  ///
  /// @note initialization is done here (sorting and update)
  ///
  /// @param gctx is the geometry context
  /// @param nState is the navigation state
  void update(const GeometryContext& gctx, NavigationState& nState) const {
    fill(gctx, nState);
    intitializeCandidates(gctx, nState);
  }

  /// Cast into a lookup position
  ///
  /// @param position is the position of the update call
  std::array<ActsScalar, grid_type::DIM> castPosition(
      const Vector3& position) const {
    std::array<ActsScalar, grid_type::DIM> casted{};
    fillCasts(position, casted,
              std::make_integer_sequence<std::size_t, grid_type::DIM>{});
    return casted;
  }

  /// Resolve duplicate on surface candidates
  ///
  /// @param gctx The geometry context of the current geometry
  /// @param surfaces is the surface candidates to check and resolve for duplicates
  void resolveDuplicates(const GeometryContext& gctx,
                         std::vector<const Acts::Surface*>& surfaces) const {
    // sorting the surfaces according to their radial distance
    std::ranges::sort(surfaces, {}, [&gctx](const auto& s) {
      assert(s != nullptr && "Uninitialized surface");
      const auto& center = s->center(gctx);
      return std::make_tuple(center.x(), center.y(), center.z());
    });

    // Remove the duplicates
    surfaces.erase(std::unique(surfaces.begin(), surfaces.end()),
                   surfaces.end());
  }

 private:
  /// Unroll the cast loop
  /// @param position is the position of the update call
  /// @param a is the array to be filled
  template <typename Array, std::size_t... idx>
  void fillCasts(const Vector3& position, Array& a,
                 std::index_sequence<idx...> /*indices*/) const {
    ((a[idx] = VectorHelpers::cast(position, casts[idx])), ...);
  }
};

/// A path generator that generates a straight path along a direction
/// in the grid
struct PathGridSurfacesGenerator {
  std::vector<Vector3> operator()(Vector3 startPosition,
                                  const Vector3& direction, ActsScalar stepSize,
                                  std::size_t numberOfSteps) const {
    std::vector<Vector3> pathCoordinates = {};
    pathCoordinates.reserve(numberOfSteps);

    Vector3 position = std::move(startPosition);
    Vector3 step = stepSize * direction;

    for (std::size_t i = 0; i < numberOfSteps; i++) {
      pathCoordinates.push_back(position);
      position = position + step;
    }

    return pathCoordinates;
  }
};

}  // namespace Acts::Experimental
