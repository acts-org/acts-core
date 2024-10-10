// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/IndexedSurfacesJsonConverter.hpp"

#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Plugins/Json/GridJsonConverter.hpp"
#include "Acts/Plugins/Json/IndexedGridJsonHelper.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"

#include <array>
#include <memory>
#include <tuple>
#include <vector>

namespace {

/// @brief  The generator struct
struct IndexedSurfacesGenerator {
  using value_type = std::vector<std::size_t>;

  /// @brief  Helper function to create and connect the IndexedSurfacesNavigation
  ///
  /// @tparam grid_type the type of the grid, indicates also the dimension
  ///
  /// @param grid the grid object
  /// @param bv the bin value array
  /// @param transform the transform for the indexed surfaces inmplementaiton
  ///
  /// @return a connected InternalNavigationDelegate object
  template <typename grid_type>
  Acts::Experimental::InternalNavigationDelegate createUpdater(
      grid_type&& grid,
      const std::array<Acts::BinningValue, grid_type::DIM>& bv,
      const Acts::Transform3& transform) {
    Acts::Experimental::IndexedSurfacesNavigation<grid_type> indexedSurfaces(
        std::forward<grid_type>(grid), bv, transform);

    // The portal delegate
    Acts::Experimental::AllPortalsNavigation allPortals;

    // The chained delegate: indexed surfaces and all portals
    using DelegateType =
        Acts::Experimental::IndexedSurfacesAllPortalsNavigation<
            grid_type, Acts::Experimental::IndexedSurfacesNavigation>;
    auto indexedSurfacesAllPortals = std::make_unique<const DelegateType>(
        std::tie(allPortals, indexedSurfaces));

    // Create the delegate and connect it
    Acts::Experimental::InternalNavigationDelegate nStateUpdater;
    nStateUpdater.connect<&DelegateType::update>(
        std::move(indexedSurfacesAllPortals));

    return nStateUpdater;
  }
};

}  // namespace

Acts::Experimental::InternalNavigationDelegate
Acts::IndexedSurfacesJsonConverter::fromJson(
    const nlohmann::json& jSurfaceNavigation) {
  if (!jSurfaceNavigation.is_null()) {
    // The return object
    auto sfCandidates = IndexedGridJsonHelper::generateFromJson<
        Experimental::InternalNavigationDelegate, IndexedSurfacesGenerator>(
        jSurfaceNavigation, "IndexedSurfaces");
    if (sfCandidates.connected()) {
      return sfCandidates;
    }
  }
  // Return the object
  return Experimental::tryAllPortalsAndSurfaces();
}
