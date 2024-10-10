// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Navigation/NavigationStateFillers.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <stdexcept>

namespace Acts::Experimental {

class DetectorVolume;

/// @brief  The end of world sets the volume pointer of the
/// navigation state to nullptr, usually indicates the end of
/// the known world, hence the name
struct EndOfWorld : public IExternalNavigation {
  /// @brief a null volume link - explicitly
  ///
  /// @note the method parameters are ignored
  inline void update(const GeometryContext& /*ignored*/,
                     NavigationState& nState) const {
    nState.currentVolume = nullptr;
  }
};

/// Navigation to a connected single detector volumed
using SingleDetectorVolumeNavigation =
    SingleObjectNavigation<IExternalNavigation, DetectorVolume,
                           DetectorVolumeFiller>;

using SingleIndex = std::size_t;

using VariableBoundAxis =
    Acts::Axis<Acts::AxisType::Variable, Acts::AxisBoundaryType::Bound>;
using VariableBoundIndexGrid1 = Acts::Grid<SingleIndex, VariableBoundAxis>;

/// @brief This holds and extracts a collection of detector
/// volumes. Alternatively an extractor could also use the
/// detector and its associated detector volume container for
/// volume extraction.
///
struct DetectorVolumesCollection {
  /// The volumes held by this collection
  std::vector<const DetectorVolume*> dVolumes = {};

  /// Extract a voume from a collection
  ///
  /// @note that geometry context and navigation state are ignored here
  /// @param index are access indices into the surfaces store
  ///
  /// @return the extracted volume
  inline const DetectorVolume* extract(const GeometryContext& /*gctx*/,
                                       const NavigationState& /*nState*/,
                                       const SingleIndex& index) const {
    return dVolumes[index];
  }
};

/// @brief This is used for volumes that are indexed in a bound
/// 1-dimensional grid, e.g. a z-spaced array, or an r-spaced array
/// of volumes.
///
struct BoundVolumesGrid1Navigation : public IExternalNavigation {
  using IndexedUpdater =
      IndexedGridNavigation<IExternalNavigation, VariableBoundIndexGrid1,
                            DetectorVolumesCollection, DetectorVolumeFiller>;
  // The indexed updator
  IndexedUpdater indexedUpdater;

  /// Allowed constructor with explicit arguments
  ///
  /// @param gBoundaries the grid boundaries
  /// @param bValue the binning value
  /// @param cVolumes the contained volumes
  /// @param bTransform is the optional transform
  BoundVolumesGrid1Navigation(
      const std::vector<ActsScalar>& gBoundaries, BinningValue bValue,
      const std::vector<const DetectorVolume*>& cVolumes,
      const Transform3& bTransform = Transform3::Identity()) noexcept(false)
      : indexedUpdater(IndexedUpdater(VariableBoundIndexGrid1(std::make_tuple(
                                          VariableBoundAxis(gBoundaries))),
                                      {bValue}, bTransform)) {
    indexedUpdater.extractor.dVolumes = cVolumes;

    if (gBoundaries.size() != cVolumes.size() + 1u) {
      throw std::invalid_argument(
          "ExternalNavigationDelegates: mismatching boundaries and volume "
          "numbers");
    }
    // Initialize the grid entries
    for (std::size_t ib = 1u; ib < gBoundaries.size(); ++ib) {
      indexedUpdater.grid.at(ib) = ib - 1;
    }
  }
  // Deleted default constructor
  BoundVolumesGrid1Navigation() = delete;

  /// @brief This updator relies on an 1D single index grid
  ///
  /// @param gctx the geometry context
  /// @param nState [in,out] the navigation state to be updated
  inline void update(const GeometryContext& gctx,
                     NavigationState& nState) const {
    indexedUpdater.update(gctx, nState);
  }
};

}  // namespace Acts::Experimental
