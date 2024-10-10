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
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/MultiLayerNavigation.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Navigation/NavigationStateFillers.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <tuple>

namespace Acts::Experimental {

struct AllPortalsNavigation : public IInternalNavigation {
  /// Fills all portals into the navigation state
  ///
  /// @param gctx is the Geometry context of this call
  /// @param nState is the navigation state to be updated
  ///
  /// @note no intersection ordering is done at this stage
  inline void fill([[maybe_unused]] const GeometryContext& gctx,
                   NavigationState& nState) const {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "AllPortalsNavigation: no detector volume set to navigation state.");
    }
    // Fill internal portals if existing
    for (const auto v : nState.currentVolume->volumes()) {
      const auto& iPortals = v->portals();
      PortalsFiller::fill(nState, iPortals);
    }
    // Filling the new portal candidates
    const auto& portals = nState.currentVolume->portals();
    PortalsFiller::fill(nState, portals);
  }

  /// A ordered portal provider - update method that calls fill and initialize
  ///
  /// @param gctx is the Geometry context of this call
  /// @param nState is the navigation state to be updated
  ///
  /// @note that the intersections are ordered, such that the
  /// smallest intersection pathlength >= overstep tolerance is the lowest
  inline void update(const GeometryContext& gctx,
                     NavigationState& nState) const {
    fill(gctx, nState);
    intitializeCandidates(gctx, nState);
  }
};

struct AllPortalsAndSurfacesNavigation : public IInternalNavigation {
  /// Fills all portals and surfaces into the navigation state
  ///
  /// @param gctx is the Geometry context of this call
  /// @param nState is the navigation state to be updated
  ///
  /// @note no intersection ordering is done at this stage
  inline void fill([[maybe_unused]] const GeometryContext& gctx,
                   NavigationState& nState) const {
    if (nState.currentDetector == nullptr) {
      throw std::runtime_error(
          "AllPortalsAndSurfacesNavigation: no detector volume set to "
          "navigation "
          "state.");
    }
    // A volume switch has happened, update list of portals & surfaces
    for (const auto v : nState.currentVolume->volumes()) {
      const auto& iPortals = v->portals();
      PortalsFiller::fill(nState, iPortals);
    }
    // Assign the new volume
    const auto& portals = nState.currentVolume->portals();
    const auto& surfaces = nState.currentVolume->surfaces();
    PortalsFiller::fill(nState, portals);
    SurfacesFiller::fill(nState, surfaces);
  }

  /// A ordered list of portals and surfaces provider
  /// - update method that calls fill and initialize
  ///
  /// @param gctx is the Geometry context of this call
  /// @param nState is the navigation state to be updated
  ///
  /// @note that the intersections are ordered, such that the
  /// smallest intersection pathlength >= overstep tolerance is the lowest
  ///
  /// @return an ordered list of portal and surface candidates
  inline void update(const GeometryContext& gctx,
                     NavigationState& nState) const {
    fill(gctx, nState);
    intitializeCandidates(gctx, nState);
  }
};

/// Generate a provider for all portals
///
/// @return a connected navigationstate updator
inline static InternalNavigationDelegate tryAllPortals() {
  auto ap = std::make_unique<const AllPortalsNavigation>();
  InternalNavigationDelegate nStateUpdater;
  nStateUpdater.connect<&AllPortalsNavigation::update>(std::move(ap));
  return nStateUpdater;
}

/// Generate a provider for all portals and Surfacess
///
/// @note this is a try-and error navigation, not recommended for production
/// setup with many surfaces
///
/// @return a connected navigationstate updator
inline static InternalNavigationDelegate tryAllPortalsAndSurfaces() {
  auto aps = std::make_unique<const AllPortalsAndSurfacesNavigation>();
  InternalNavigationDelegate nStateUpdater;
  nStateUpdater.connect<&AllPortalsAndSurfacesNavigation::update>(
      std::move(aps));
  return nStateUpdater;
}

/// @brief This holds and extracts a collection of surfaces without much
/// checking, this could be e.g. support surfaces for layer structures,
/// e.g.
///
struct AdditionalSurfacesNavigation : public IInternalNavigation {
  /// The volumes held by this collection
  std::vector<const Surface*> surfaces = {};

  /// Extract the additional surfaces from the this volume
  ///
  /// @param gctx the geometry contextfor this extraction call (ignored)
  /// @param nState is the navigation state
  ///
  /// @note no intersection ordering is done at this stage
  inline void fill([[maybe_unused]] const GeometryContext& gctx,
                   NavigationState& nState) const {
    SurfacesFiller::fill(nState, surfaces);
  }

  /// Extract the additional surfaces from the this volume
  /// - update method that calls fill and initialize
  ///
  /// @param gctx is the Geometry context of this call
  /// @param nState is the navigation state to be updated
  ///
  /// @note that the intersections are ordered, such that the
  /// smallest intersection pathlength >= overstep tolerance is the lowest
  ///
  /// @return an ordered list of portal and surface candidates
  inline void update(const GeometryContext& gctx,
                     NavigationState& nState) const {
    fill(gctx, nState);
    intitializeCandidates(gctx, nState);
  }
};

/// @brief  An indexed surface implementation access
///
/// @tparam grid_type is the grid type used for this indexed lookup
template <typename grid_type>
using IndexedSurfacesNavigation =
    IndexedGridNavigation<IInternalNavigation, grid_type,
                          IndexedSurfacesExtractor, SurfacesFiller>;

/// @brief  An indexed multi layer surface implementation access
///
/// @tparam grid_type is the grid type used for this indexed lookup
template <typename grid_type>
using MultiLayerSurfacesNavigation =
    MultiLayerNavigation<grid_type, PathGridSurfacesGenerator>;

/// @brief An indexed surface implementation with portal access
///
///@tparam inexed_updator is the updator for the indexed surfaces
template <typename grid_type, template <typename> class indexed_updator>
using IndexedSurfacesAllPortalsNavigation =
    ChainedNavigation<IInternalNavigation, AllPortalsNavigation,
                      indexed_updator<grid_type>>;

}  // namespace Acts::Experimental
