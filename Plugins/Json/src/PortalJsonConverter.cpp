// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/PortalJsonConverter.hpp"

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/detail/PortalHelper.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace {

/// Find the position of the volume to point to
///
/// @param volume the volume to find
/// @param the collection of volumes
///
/// @note return -1 if not found, to be interpreted by the caller
int findVolume(
    const Acts::Experimental::DetectorVolume* volume,
    const std::vector<const Acts::Experimental::DetectorVolume*>& volumes) {
  auto candidate = std::ranges::find(volumes, volume);
  if (candidate != volumes.end()) {
    return std::distance(volumes.begin(), candidate);
  }
  return -1;
}
}  // namespace

nlohmann::json Acts::PortalJsonConverter::toJson(
    const GeometryContext& gctx, const Experimental::Portal& portal,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes,
    const Options& options) {
  // The overall return object
  nlohmann::json jPortal;
  // Write the surface
  jPortal["surface"] = SurfaceJsonConverter::toJson(gctx, portal.surface(),
                                                    options.surfaceOptions);
  // And the portal specific information
  const auto& volumeLinks = portal.portalNavigation();
  nlohmann::json jLinks;
  for (const auto& vLink : volumeLinks) {
    nlohmann::json jLink = toJson(vLink, detectorVolumes);
    jLinks.push_back(jLink);
  }
  jPortal["volume_links"] = jLinks;
  // Return the full json object
  return jPortal;
}

std::tuple<std::vector<nlohmann::json>,
           std::vector<std::shared_ptr<Acts::Surface>>>
Acts::PortalJsonConverter::toJsonDetray(
    const GeometryContext& gctx, const Experimental::Portal& portal,
    std::size_t ip, const Experimental::DetectorVolume& volume,
    const std::vector<OrientedSurface>& orientedSurfaces,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes,
    const Options& option) {
  // The overall return object
  std::vector<nlohmann::json> jPortals = {};
  const RegularSurface& surface = portal.surface();
  const auto& volumeLinks = portal.portalNavigation();

  // First assumption for outside link (along direction)
  std::size_t outside = 1u;

  std::vector<std::shared_ptr<Acts::Surface>> subSurfaces = {};

  // Find out if you need to take the outside or inside volume
  // for planar surfaces that's easy
  if (surface.type() != Acts::Surface::SurfaceType::Cylinder) {
    // Get the two volume center
    const auto volumeCenter = volume.transform(gctx).translation();
    const auto surfaceCenter = surface.center(gctx);
    const auto surfaceNormal = surface.normal(gctx, surfaceCenter);
    // Get the direction from the volume to the surface, correct link
    const auto volumeToSurface = surfaceCenter - volumeCenter;
    if (volumeToSurface.dot(surfaceNormal) < 0.) {
      outside = 0u;
    }
  } else {
    // This is a cylinder portal, inner cover reverses the normal
    if (ip == 3u) {
      outside = 0u;
    }
  }

  const auto& outsideLink = volumeLinks[outside];
  // Grab the corresponding volume link
  // If it is a single link, we are done
  const auto* instance = outsideLink.instance();
  // Single link cast
  auto singleLink =
      dynamic_cast<const Acts::Experimental::SingleDetectorVolumeNavigation*>(
          instance);

  auto [surfaceAdjusted, insidePointer] = orientedSurfaces[ip];

  SurfaceJsonConverter::Options surfaceOptions = option.surfaceOptions;
  surfaceOptions.portal = true;
  // Single link detected - just write it out, we use the oriented surface
  // in order to make sure the size is adjusted
  if (singleLink != nullptr) {
    // Single link can be written out
    std::size_t vLink = findVolume(singleLink->object(), detectorVolumes);
    auto jPortal = SurfaceJsonConverter::toJsonDetray(gctx, *surfaceAdjusted,
                                                      surfaceOptions);
    DetrayJsonHelper::addVolumeLink(jPortal["mask"], vLink);
    jPortals.push_back(jPortal);
  } else {
    // Multi link detected - 1D
    auto multiLink1D =
        dynamic_cast<const Experimental::BoundVolumesGrid1Navigation*>(
            instance);
    if (multiLink1D != nullptr) {
      // Resolve the multi link 1D
      auto boundaries =
          multiLink1D->indexedUpdater.grid.axes()[0u]->getBinEdges();
      const auto& cast = multiLink1D->indexedUpdater.casts[0u];
      const auto& transform = multiLink1D->indexedUpdater.transform;
      const auto& volumes = multiLink1D->indexedUpdater.extractor.dVolumes;

      // Apply the correction from local to global boundaries
      ActsScalar gCorr = VectorHelpers::cast(transform.translation(), cast);
      std::for_each(boundaries.begin(), boundaries.end(),
                    [&gCorr](ActsScalar& b) { b -= gCorr; });

      // Get the volume indices
      auto surfaceType = surfaceAdjusted->type();
      std::vector<unsigned int> vIndices = {};
      for (const auto& v : volumes) {
        vIndices.push_back(findVolume(v, detectorVolumes));
      }

      // Pick the surface dimension - via poly
      std::array<ActsScalar, 2u> clipRange = {0., 0.};
      std::vector<ActsScalar> boundValues = surfaceAdjusted->bounds().values();
      if (surfaceType == Surface::SurfaceType::Cylinder &&
          cast == BinningValue::binZ) {
        ActsScalar zPosition = surfaceAdjusted->center(gctx).z();
        clipRange = {
            zPosition - boundValues[CylinderBounds::BoundValues::eHalfLengthZ],
            zPosition + boundValues[CylinderBounds::BoundValues::eHalfLengthZ]};
      } else if (surfaceType == Surface::SurfaceType::Disc &&
                 cast == BinningValue::binR) {
        clipRange = {boundValues[RadialBounds::BoundValues::eMinR],
                     boundValues[RadialBounds::BoundValues::eMaxR]};
      } else {
        throw std::runtime_error(
            "PortalJsonConverter: surface type not (yet) supported for detray "
            "conversion, only cylinder and disc are currently supported.");
      }

      // Need to clip the parameter space to the surface dimension
      std::vector<unsigned int> clippedIndices = {};
      std::vector<ActsScalar> clippedBoundaries = {};
      clippedBoundaries.push_back(clipRange[0u]);
      for (const auto [ib, b] : enumerate(boundaries)) {
        if (ib > 0) {
          unsigned int vI = vIndices[ib - 1u];
          ActsScalar highEdge = boundaries[ib];
          if (boundaries[ib - 1] >= clipRange[1u]) {
            break;
          }
          if (highEdge <= clipRange[0u] ||
              std::abs(highEdge - clipRange[0u]) < 1e-5) {
            continue;
          }
          if (highEdge > clipRange[1u]) {
            highEdge = clipRange[1u];
          }
          clippedIndices.push_back(vI);
          clippedBoundaries.push_back(highEdge);
        }
      }
      // Interpret the clipped information
      //
      // Clipped cylinder case
      if (surfaceType == Surface::SurfaceType::Cylinder) {
        for (auto [ib, b] : enumerate(clippedBoundaries)) {
          if (ib > 0) {
            // Create sub surfaces
            std::array<ActsScalar, CylinderBounds::BoundValues::eSize>
                subBoundValues = {};
            for (auto [ibv, bv] : enumerate(boundValues)) {
              subBoundValues[ibv] = bv;
            }
            subBoundValues[CylinderBounds::BoundValues::eHalfLengthZ] =
                0.5 * (b - clippedBoundaries[ib - 1u]);
            auto subBounds = std::make_shared<CylinderBounds>(subBoundValues);
            auto subTransform = Transform3::Identity();
            subTransform.pretranslate(Vector3(
                0., 0.,
                clippedBoundaries[ib - 1u] +
                    subBoundValues[CylinderBounds::BoundValues::eHalfLengthZ]));
            auto subSurface =
                Surface::makeShared<CylinderSurface>(subTransform, subBounds);
            subSurfaces.push_back(subSurface);
            auto jPortal = SurfaceJsonConverter::toJsonDetray(gctx, *subSurface,
                                                              surfaceOptions);
            DetrayJsonHelper::addVolumeLink(jPortal["mask"],
                                            clippedIndices[ib - 1u]);
            jPortals.push_back(jPortal);
          }
        }
      } else {
        for (auto [ib, b] : enumerate(clippedBoundaries)) {
          if (ib > 0) {
            // Create sub surfaces
            std::array<ActsScalar, RadialBounds::BoundValues::eSize>
                subBoundValues = {};
            for (auto [ibv, bv] : enumerate(boundValues)) {
              subBoundValues[ibv] = bv;
            }
            subBoundValues[RadialBounds::BoundValues::eMinR] =
                clippedBoundaries[ib - 1u];
            subBoundValues[RadialBounds::BoundValues::eMaxR] = b;
            auto subBounds = std::make_shared<RadialBounds>(subBoundValues);
            auto subSurface = Surface::makeShared<DiscSurface>(
                portal.surface().transform(gctx), subBounds);
            subSurfaces.push_back(subSurface);
            auto jPortal = SurfaceJsonConverter::toJsonDetray(gctx, *subSurface,
                                                              surfaceOptions);
            DetrayJsonHelper::addVolumeLink(jPortal["mask"],
                                            clippedIndices[ib - 1u]);
            jPortals.push_back(jPortal);
          }
        }
      }

    } else {
      // End of world
      // Write surface with invalid link
      auto jPortal = SurfaceJsonConverter::toJsonDetray(gctx, *surfaceAdjusted,
                                                        surfaceOptions);
      DetrayJsonHelper::addVolumeLink(
          jPortal["mask"], std::numeric_limits<std::uint_least16_t>::max());
      jPortals.push_back(jPortal);
    }
  }
  return {jPortals, subSurfaces};
}

nlohmann::json Acts::PortalJsonConverter::toJson(
    const Experimental::ExternalNavigationDelegate& updator,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes) {
  nlohmann::json jLink;
  if (updator.connected()) {
    const auto instance = updator.instance();
    // Single link cast
    auto singleLink =
        dynamic_cast<const Experimental::SingleDetectorVolumeNavigation*>(
            instance);
    // Single link detected
    if (singleLink != nullptr) {
      auto vIndex = findVolume(singleLink->object(), detectorVolumes);
      if (vIndex < 0) {
        throw std::runtime_error(
            "PortalJsonConverter: volume not found in the list of volumes.");
      }
      jLink["single"] = vIndex;
    }
    // Multi link detected - 1D
    auto multiLink1D =
        dynamic_cast<const Experimental::BoundVolumesGrid1Navigation*>(
            instance);
    if (multiLink1D != nullptr) {
      nlohmann::json jMultiLink;
      const auto& volumes = multiLink1D->indexedUpdater.extractor.dVolumes;
      const auto& casts = multiLink1D->indexedUpdater.casts;
      nlohmann::json jTransform = Transform3JsonConverter::toJson(
          multiLink1D->indexedUpdater.transform);
      std::vector<unsigned int> vIndices = {};
      for (const auto& v : volumes) {
        vIndices.push_back(findVolume(v, detectorVolumes));
      }
      jMultiLink["boundaries"] =
          multiLink1D->indexedUpdater.grid.axes()[0u]->getBinEdges();
      jMultiLink["binning"] = casts[0u];
      jMultiLink["targets"] = vIndices;
      jMultiLink["transform"] = jTransform;
      jLink["multi_1D"] = jMultiLink;
    }
  }
  return jLink;
}

std::shared_ptr<Acts::Experimental::Portal> Acts::PortalJsonConverter::fromJson(
    const GeometryContext& gctx, const nlohmann::json& jPortal,
    const std::vector<std::shared_ptr<Experimental::DetectorVolume>>&
        detectorVolumes) {
  // The surface re-creation is trivial
  auto surface = SurfaceJsonConverter::fromJson(jPortal["surface"]);
  auto regSurface = std::dynamic_pointer_cast<RegularSurface>(surface);
  if (!regSurface) {
    throw std::runtime_error(
        "PortalJsonConverter: surface is not a regular surface.");
  }
  auto portal = std::make_shared<Experimental::Portal>(regSurface);

  std::array<Acts::Direction, 2> normalDirs = {Direction::Backward,
                                               Direction::Forward};
  // re-create the volume links
  auto jLinks = jPortal["volume_links"];
  for (auto [ivl, vl] : enumerate(jLinks)) {
    if (vl.contains("single")) {
      const auto vIndex = vl["single"].get<unsigned int>();
      Experimental::detail::PortalHelper::attachExternalNavigationDelegate(
          *portal, detectorVolumes[vIndex], normalDirs[ivl]);
    } else if (vl.contains("multi_1D")) {
      // Resolve the multi link 1D
      auto jMultiLink = vl["multi_1D"];
      auto boundaries = jMultiLink["boundaries"].get<std::vector<double>>();
      auto binning = jMultiLink["binning"].get<BinningValue>();
      auto targets = jMultiLink["targets"].get<std::vector<unsigned int>>();
      std::vector<std::shared_ptr<Experimental::DetectorVolume>> targetVolumes;
      for (const auto t : targets) {
        targetVolumes.push_back(detectorVolumes[t]);
      }
      Experimental::detail::PortalHelper::attachDetectorVolumesUpdater(
          gctx, *portal, targetVolumes, normalDirs[ivl], boundaries, binning);
    }
  }

  return portal;
}
