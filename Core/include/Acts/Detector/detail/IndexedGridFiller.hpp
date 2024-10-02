// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/GridAccessHelpers.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/IAxis.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <vector>

namespace Acts::Experimental::detail {

/// @brief Helper method to generate completely populated bin sequences
/// that respect the boundary type of the axis
///
/// @param minMaxBins estimated bin range (aka binning boundary box)
/// @param expand the parameter to expand the view (extra window)
/// @param nBins the maximum number of bins on this axis
/// @param type the boundary type of the axis (for correct bin closure)
///
/// @note for closed binning a span over half the bins flips direction
///
/// @return a vector of bins to be filled
std::vector<std::size_t> binSequence(std::array<std::size_t, 2u> minMaxBins,
                                     std::size_t expand, std::size_t nBins,
                                     Acts::AxisBoundaryType type);

/// @brief Helper method to fill local bins given a set of query points
/// bin in between the extra points are filled, and a possible expansion
/// of the bin window can be chosen
///
/// @tparam grid_type the type of the grid that determines locall binning
///
/// @param grid the grid used for this
/// @param queries the grid positions for the bin queries
/// @param expansion are the additional (configured) number of bins to expand
/// the view
///
/// @return a set of unique indices
template <typename grid_type>
std::set<typename grid_type::index_t> localIndices(
    const grid_type& grid,
    const std::vector<typename grid_type::point_t>& queries,
    const std::vector<std::size_t>& expansion = {}) {
  // Return indices
  std::set<typename grid_type::index_t> lIndices;

  if (queries.empty()) {
    throw std::runtime_error("IndexedSurfaceGridFiller: no query point given.");
  }

  if (!expansion.empty() && expansion.size() != grid_type::DIM) {
    throw std::runtime_error(
        "IndexedSurfaceGridFiller: wrong dimension of bin expansion given.");
  }

  /// These are the axis bounds type parameters - for correct bin sequences
  std::array<Acts::AxisBoundaryType, grid_type::DIM> axisTypes{};
  std::array<std::size_t, grid_type::DIM> axisBins{};
  // Fill the axis types
  for (auto [ia, a] : enumerate(grid.axes())) {
    axisTypes[ia] = a->getBoundaryType();
    axisBins[ia] = a->getNBins();
  }

  // Initialize the bin ranges
  std::array<std::array<std::size_t, 2u>, grid_type::DIM> binRanges = {};
  for (auto& br : binRanges) {
    br[0u] = std::numeric_limits<std::size_t>::max();
    br[1u] = 0u;
  }
  // Bin range bounding box - estimated from the query points
  for (const auto& q : queries) {
    auto qbin = grid.localBinsFromPosition(q);
    for (std::size_t ib = 0; ib < grid_type::DIM; ++ib) {
      auto iqb = qbin[ib];
      binRanges[ib][0u] = std::min(iqb, binRanges[ib][0u]);
      binRanges[ib][1u] = std::max(iqb, binRanges[ib][1u]);
    }
  }
  // Fill the bins - 1D case
  if constexpr (grid_type::DIM == 1u) {
    // Take the expansion if available & generate the local bin sequence
    std::size_t expand = expansion.empty() ? 0u : expansion[0u];
    auto localBins0 =
        binSequence(binRanges[0u], expand, axisBins[0u], axisTypes[0u]);
    for (auto l0 : localBins0) {
      typename grid_type::index_t b;
      b[0u] = l0;
      lIndices.insert(b);
    }
  }
  // Fill the bins - 2D case
  if constexpr (grid_type::DIM == 2u) {
    // Take the expansion if available & generate the local bin sequence
    std::size_t expand = expansion.empty() ? 0u : expansion[0u];
    auto localBins0 =
        binSequence(binRanges[0u], expand, axisBins[0u], axisTypes[0u]);
    expand = expansion.empty() ? 0u : expansion[1u];
    auto localBins1 =
        binSequence(binRanges[1u], expand, axisBins[1u], axisTypes[1u]);
    for (auto l0 : localBins0) {
      for (auto l1 : localBins1) {
        typename grid_type::index_t b;
        b[0u] = l0;
        b[1u] = l1;
        lIndices.insert(b);
      }
    }
  }
  return lIndices;
}

/// @brief Helper method to screen output the local bins
///
/// @tparam local_bin the type of the local bins
///
/// @param lbins the local bins
///
/// @return a string containing the local bins ordered in a set
template <typename local_bin>
std::string outputIndices(const std::set<local_bin>& lbins) {
  std::string rString;
  for (auto [ilb, lb] : Acts::enumerate(lbins)) {
    if (ilb == 0) {
      rString = "bins: [";
    } else {
      rString += ", [";
    }
    for (auto [ib, b] : Acts::enumerate(lb)) {
      if (ib != 0u) {
        rString += ", ";
      }
      rString += std::to_string(b);
    }
    rString += "]";
  }
  return rString;
}

/// A helper class that fills surfaces into predefined grids
struct IndexedGridFiller {
  /// Bin expansion where needed
  std::vector<std::size_t> binExpansion = {};

  /// Screen output logger
  std::unique_ptr<const Logger> oLogger =
      getDefaultLogger("IndexedGridFiller", Logging::INFO);

  /// @brief This method takes a collection of objects and fills them
  /// into an index grid - it uses a reference generator for grid query points
  /// and then completes the bins in between.
  ///
  /// It also allows for expanding the fill view.
  ///
  /// @tparam index_grid the type of the index grid
  /// @tparam indexed_objects the type of the object container
  /// @tparam reference_generator the generator for reference points to be filled
  ///
  /// @param gctx the geometry context of the operation
  /// @param iGrid [in,out] the index grid object to be filled
  /// @param iObjects the object container to be indexed
  /// @param rGenerator the reference point generator for position queries
  /// @param aToAll the indices that are assigned to all bins
  ///
  /// @note as this is a Detector module, the objects within the indexed_objects container
  /// are assumed to have pointer semantics
  ///
  template <typename index_grid, typename indexed_objects,
            typename reference_generator>
  void fill(
      const GeometryContext& gctx, index_grid& iGrid,
      const indexed_objects& iObjects, const reference_generator& rGenerator,
      const typename index_grid::grid_type::value_type& aToAll = {}) const {
    // Loop over the surfaces to be filled
    for (auto [io, o] : enumerate(iObjects)) {
      // Exclude indices that should be handled differently
      if (rangeContainsValue(aToAll, io)) {
        continue;
      }
      // Get the reference positions
      auto refs = rGenerator.references(gctx, *o);
      std::vector<typename index_grid::grid_type::point_t> gridQueries;
      gridQueries.reserve(refs.size());
      for (const auto& ref : refs) {
        // Cast the transform according to the grid binning
        gridQueries.push_back(
            GridAccessHelpers::castPosition<decltype(iGrid.grid)>(
                iGrid.transform * ref, iGrid.casts));
      }
      ACTS_DEBUG(gridQueries.size() << " reference points generated.");
      auto lIndices = localIndices<decltype(iGrid.grid)>(
          iGrid.grid, gridQueries, binExpansion);
      ACTS_DEBUG(lIndices.size() << " indices assigned.");
      if (oLogger->level() <= Logging::VERBOSE) {
        ACTS_VERBOSE("- list of indices: " << outputIndices(lIndices));
      }
      // Now fill the surface indices
      for (const auto& li : lIndices) {
        auto& bContent = iGrid.grid.atLocalBins(li);
        if (!rangeContainsValue(bContent, io)) {
          bContent.push_back(io);
        }
      }
    }

    // Assign the indices into all
    if (!aToAll.empty()) {
      assignToAll(iGrid, aToAll);
    }
  }

  /// Helper method to fill a dedicated list of indices into the entire grid
  ///
  /// This is useful if e.g. certain objects are to be attempted in any case,
  /// regardless of their binning.
  ///
  template <typename index_grid, typename indices>
  void assignToAll(index_grid& iGrid, const indices& idcs) const {
    for (std::size_t gi = 0; gi < iGrid.grid.size(true); ++gi) {
      auto& bContent = iGrid.grid.at(gi);
      for (const auto& io : idcs) {
        if (!rangeContainsValue(bContent, io)) {
          bContent.push_back(io);
        }
      }
    }
  }

  /// Access to the logger
  ///
  /// @return a const reference to the logger
  const Logger& logger() const { return (*oLogger); }
};

}  // namespace Acts::Experimental::detail
