// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Navigation/NavigationStateFillers.hpp"
#include "Acts/Navigation/NavigationStateUpdaters.hpp"
#include "Acts/Utilities/Axis.hpp"
#include "Acts/Utilities/Grid.hpp"

#include <stdexcept>

namespace Acts::Experimental {

struct NoopFinder : public IExternalNavigation {
  inline void update(const GeometryContext& /*gctx*/,
                     NavigationState& /*nState*/) const {}
};

struct RootVolumeFinder : public IExternalNavigation {
  inline void update(const GeometryContext& gctx,
                     NavigationState& nState) const {
    if (nState.currentDetector == nullptr) {
      throw std::runtime_error(
          "DetectorVolumeFinders: no detector set to navigation state.");
    }

    const auto& volumes = nState.currentDetector->rootVolumes();
    for (const auto v : volumes) {
      if (v->inside(gctx, nState.position)) {
        nState.currentVolume = v;
        v->externalNavigation()(gctx, nState);
        return;
      }
    }
    nState.currentVolume = nullptr;
  }
};

struct TrialAndErrorVolumeFinder : public IExternalNavigation {
  inline void update(const GeometryContext& gctx,
                     NavigationState& nState) const {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "DetectorVolumeFinders: no volume set to navigation state.");
    }

    const auto& volumes = nState.currentVolume->volumes();
    for (const auto v : volumes) {
      if (v->inside(gctx, nState.position)) {
        nState.currentVolume = v;
        v->externalNavigation()(gctx, nState);
        return;
      }
    }
  }
};

/// Generate a delegate to try the root volumes
inline static ExternalNavigationDelegate tryRootVolumes() {
  ExternalNavigationDelegate vFinder;
  vFinder.connect<&RootVolumeFinder::update>(
      std::make_unique<const RootVolumeFinder>());
  return vFinder;
}

/// Generate a delegate to try all sub volumes
inline static ExternalNavigationDelegate tryAllSubVolumes() {
  ExternalNavigationDelegate vFinder;
  vFinder.connect<&TrialAndErrorVolumeFinder::update>(
      std::make_unique<const TrialAndErrorVolumeFinder>());
  return vFinder;
}

/// Generate a delegate to try no volume
inline static ExternalNavigationDelegate tryNoVolumes() {
  ExternalNavigationDelegate vFinder;
  vFinder.connect<&NoopFinder::update>(std::make_unique<const NoopFinder>());
  return vFinder;
}

/// @brief A helper struct that allows to extrace a volume
/// from the detector by its index
struct IndexedDetectorVolumeExtractor {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  /// @param index is the index in the global detector volume store
  ///
  /// @return a raw DetectorVolume pointer
  inline static const DetectorVolume* extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState, std::size_t index) noexcept(false) {
    if (nState.currentDetector == nullptr) {
      throw std::runtime_error("IndexedVolumeExtractor: no detector given.");
    }
    // Get the volume container from the detector
    const auto& volumes = nState.currentDetector->volumes();
    return volumes[index];
  }
};

/// @brief  An indexed volume implementation access
///
/// @tparam grid_type is the grid type used for this
template <typename grid_type>
using IndexedDetectorVolumesImpl =
    IndexedGridNavigation<IExternalNavigation, grid_type,
                          IndexedDetectorVolumeExtractor, DetectorVolumeFiller>;

}  // namespace Acts::Experimental
