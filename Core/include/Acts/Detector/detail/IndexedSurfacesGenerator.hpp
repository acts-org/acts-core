// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/IndexedGridFiller.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <algorithm>
#include <array>
#include <memory>

namespace Acts::Experimental::detail {

/// @brief  A templated indexed grid generator.
///
/// This Generator creates a InternalNavigationDelegate delegate
/// which can then be used in the DetectorVolume class for updating
/// given surface candidates based on an index grid.
///
/// It allows for:
/// - certain indices being forcly assigned to all bins
/// - a chosen expansion to fill indices in neighborhood bins
///
/// @tparam objects_container the objects container
template <typename surface_container, template <typename> class indexed_updator>
struct IndexedSurfacesGenerator {
  /// The surfaces to be indexed
  /// (including surfaces that are assigned to all bins)
  surface_container surfaces = {};
  // Indices of surfaces that are to be assigned to all bins
  std::vector<std::size_t> assignToAll = {};
  /// The binning for the indexing
  std::vector<BinningValue> bValues = {};
  // Bin expansion
  std::vector<std::size_t> binExpansion = {};
  /// The transform into the local binning schema
  Transform3 transform = Transform3::Identity();
  /// Screen output logger
  std::unique_ptr<const Logger> oLogger =
      getDefaultLogger("IndexedSurfacesGenerator", Logging::INFO);

  /// Create the Surface candidate updator
  ///
  /// @tparam axis_generator does generate the axis of the grid
  /// @tparam reference_generator does generate the reference query points
  ///
  /// @param gctx the geometry context
  /// @param aGenerator the axis generator
  /// @param rGenerator the reference generataor
  ///
  /// @return an InternalNavigationDelegate
  template <typename axis_generator, typename reference_generator>
  InternalNavigationDelegate operator()(
      const GeometryContext& gctx, const axis_generator& aGenerator,
      const reference_generator& rGenerator) const {
    ACTS_DEBUG("Indexing " << surfaces.size() << " surface, "
                           << assignToAll.size() << " of which into all bins.");
    // Create the grid with the provided axis generator
    using GridType =
        typename axis_generator::template grid_type<std::vector<std::size_t>>;
    GridType grid(std::move(aGenerator()));

    std::array<BinningValue, decltype(grid)::DIM> bvArray = {};
    for (auto [ibv, bv] : enumerate(bValues)) {
      bvArray[ibv] = bv;
    }

    indexed_updator<GridType> indexedSurfaces(std::move(grid), bvArray,
                                              transform);
    // Fill the bin indices
    IndexedGridFiller filler{binExpansion};
    filler.oLogger = oLogger->cloneWithSuffix("_filler");
    filler.fill(gctx, indexedSurfaces, surfaces, rGenerator, assignToAll);

    // The portal delegate
    AllPortalsNavigation allPortals;

    // The chained delegate: indexed surfaces and all portals
    using DelegateType =
        IndexedSurfacesAllPortalsNavigation<decltype(grid), indexed_updator>;
    auto indexedSurfacesAllPortals = std::make_unique<const DelegateType>(
        std::tie(allPortals, indexedSurfaces));

    // Create the delegate and connect it
    InternalNavigationDelegate nStateUpdater;
    nStateUpdater.connect<&DelegateType::update>(
        std::move(indexedSurfacesAllPortals));
    return nStateUpdater;
  }

  /// Access to the logger
  ///
  /// @return a const reference to the logger
  const Logger& logger() const { return *oLogger; }
};

}  // namespace Acts::Experimental::detail
