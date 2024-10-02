// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/detail/PortalHelper.hpp"

#include "Acts/Detector/Portal.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <array>
#include <stdexcept>
#include <utility>

void Acts::Experimental::detail::PortalHelper::attachExternalNavigationDelegate(
    Portal& portal, const std::shared_ptr<DetectorVolume>& volume,
    const Direction& direction) {
  // Create a shared link instance & delegate
  auto volumeLinkImpl = std::make_unique<
      const Acts::Experimental::SingleDetectorVolumeNavigation>(volume.get());
  Acts::Experimental::ExternalNavigationDelegate volumeLink;
  volumeLink
      .connect<&Acts::Experimental::SingleDetectorVolumeNavigation::update>(
          std::move(volumeLinkImpl));
  // Update the volume link and the store
  portal.assignPortalNavigation(direction, std::move(volumeLink), {volume});
}

void Acts::Experimental::detail::PortalHelper::attachDetectorVolumesUpdater(
    const GeometryContext& gctx, Portal& portal,
    const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    const Direction& direction, const std::vector<ActsScalar>& boundaries,
    const BinningValue& binning) {
  // Check if the boundaries need a transform
  const auto pTransform = portal.surface().transform(gctx);
  // Creating a link to the mother
  auto volumes1D = std::make_unique<const BoundVolumesGrid1Navigation>(
      boundaries, binning, unpack_shared_const_vector(volumes),
      pTransform.inverse());
  ExternalNavigationDelegate dVolumeUpdater;
  dVolumeUpdater.connect<&BoundVolumesGrid1Navigation::update>(
      std::move(volumes1D));
  portal.assignPortalNavigation(direction, std::move(dVolumeUpdater), volumes);
}

void Acts::Experimental::detail::PortalHelper::
    attachExternalNavigationDelegates(
        const GeometryContext& gctx,
        const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
        std::vector<PortalReplacement>& pReplacements) {
  // Unpack to navigation bare points
  auto cVolumes = unpack_shared_const_vector(volumes);
  // Set to the constructed portals (p), at index (i), in direction (d)
  // using boundaries and binning
  for (auto& [p, i, dir, boundaries, binning] : pReplacements) {
    // Check if the boundaries need a transform
    const auto pTransform = p->surface().transform(gctx);
    // Creating a link to the mother
    auto volumes1D = std::make_unique<const BoundVolumesGrid1Navigation>(
        boundaries, binning, cVolumes, pTransform.inverse());
    ExternalNavigationDelegate dVolumeUpdater;
    dVolumeUpdater.connect<&BoundVolumesGrid1Navigation::update>(
        std::move(volumes1D));
    p->assignPortalNavigation(dir, std::move(dVolumeUpdater), volumes);
  }
}

std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>
Acts::Experimental::detail::PortalHelper::attachedDetectorVolumes(
    Portal& portal) noexcept(false) {
  auto& attachedVolumes = portal.attachedDetectorVolumes();
  if (!attachedVolumes[0u].empty() && !attachedVolumes[1u].empty()) {
    throw std::invalid_argument(
        "PortalHelper: trying to get attachedVolumes from already populated "
        "portal.");
  }
  unsigned int iu = attachedVolumes[0u].empty() ? 1u : 0u;
  return attachedVolumes[iu];
}

std::map<unsigned int,
         std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>>
Acts::Experimental::detail::PortalHelper::stripSideVolumes(
    const std::vector<std::map<unsigned int, std::shared_ptr<Portal>>>&
        pContainers,
    const std::vector<unsigned int>& sides,
    const std::vector<unsigned int>& selectedOnly,
    Acts::Logging::Level logLevel) {
  ACTS_LOCAL_LOGGER(Acts::getDefaultLogger("::stripSideVolumes", logLevel));

  // These are the stripped off outside volumes
  std::map<unsigned int,
           std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>>
      sideVolumes;

  // Principle sides and selected sides, make an intersection
  std::vector<unsigned int> selectedSides;
  if (!selectedOnly.empty()) {
    std::set_intersection(sides.begin(), sides.end(), selectedOnly.begin(),
                          selectedOnly.end(),
                          std::back_inserter(selectedSides));
  } else {
    selectedSides = sides;
  }

  // Loop through the containers
  for (const auto& pc : pContainers) {
    // Loop through the selected sides and check if they are contained
    for (const auto& s : selectedSides) {
      auto cSide = pc.find(s);
      if (cSide != pc.end()) {
        auto p = cSide->second;
        auto& sVolumes = sideVolumes[s];
        auto aVolumes =
            Acts::Experimental::detail::PortalHelper::attachedDetectorVolumes(
                *p);
        sVolumes.insert(sVolumes.end(), aVolumes.begin(), aVolumes.end());
      }
    }
  }
  // return them
  return sideVolumes;
}
