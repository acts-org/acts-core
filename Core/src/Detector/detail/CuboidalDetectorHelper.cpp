// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Detector/detail/CuboidalDetectorHelper.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/detail/DetectorVolumeConsistency.hpp"
#include "Acts/Detector/detail/PortalHelper.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <algorithm>

Acts::Experimental::DetectorComponent::PortalContainer
Acts::Experimental::detail::CuboidalDetectorHelper::connect(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<Experimental::DetectorVolume>>& volumes,
    AxisDirection bValue, const std::vector<unsigned int>& selectedOnly,
    Acts::Logging::Level logLevel) {
  ACTS_LOCAL_LOGGER(getDefaultLogger("CuboidalDetectorHelper", logLevel));

  ACTS_DEBUG("Connect " << volumes.size() << " detector volumes in "
                        << axisDirectionName(bValue) << ".");

  // Check transform for consistency
  auto centerDistances =
      DetectorVolumeConsistency::checkCenterAlignment(gctx, volumes, bValue);

  // Assign the portal indices according to the volume bounds definition
  std::array<AxisDirection, 3u> possibleValues = {
      AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ};
  // 1 -> [ 2,3 ] for AxisX connection (cylclic one step)
  // 2 -> [ 4,5 ] for AxisY connection (cylclic two steps)
  // 0 -> [ 0,1 ] for AxisZ connection (to be in line with cylinder covnention)
  using PortalSet = std::array<std::size_t, 2u>;
  std::vector<PortalSet> portalSets = {
      {PortalSet{2, 3}, PortalSet{4, 5}, PortalSet{0, 1}}};

  // This is the picked set for fusing
  auto [sIndex, fIndex] = portalSets[toUnderlying(bValue)];

  // Log the merge splits, i.e. the boundaries of the volumes
  std::array<std::vector<double>, 3u> mergeSplits;
  std::array<double, 3u> mergeHalfLengths = {
      0.,
      0.,
      0.,
  };

  // Pick the counter part value
  auto counterPart = [&](AxisDirection mValue) -> AxisDirection {
    for (auto cValue : possibleValues) {
      if (cValue != mValue && cValue != bValue) {
        return cValue;
      }
    }
    return mValue;
  };

  // Things that can be done without a loop be first/last check
  // Estimate the merge parameters: the scalar and the transform
  using MergeParameters = std::tuple<double, Transform3>;
  std::map<std::size_t, MergeParameters> mergeParameters;
  auto& firstVolume = volumes.front();
  auto& lastVolume = volumes.back();
  // Values
  const auto firstBoundValues = firstVolume->volumeBounds().values();
  const auto lastBoundValues = lastVolume->volumeBounds().values();
  Vector3 stepDirection =
      firstVolume->transform(gctx).rotation().col(toUnderlying(bValue));

  for (auto [im, mergeValue] : enumerate(possibleValues)) {
    // Skip the bin value itself, fusing will took care of that
    if (mergeValue == bValue) {
      continue;
    }
    for (auto [is, index] : enumerate(portalSets[toUnderlying(mergeValue)])) {
      // Take rotation from first volume
      auto rotation = firstVolume->portalPtrs()[index]
                          ->surface()
                          .transform(gctx)
                          .rotation();
      double stepDown = firstBoundValues[toUnderlying(bValue)];
      double stepUp = lastBoundValues[toUnderlying(bValue)];
      // Take translation from first and last volume
      auto translationF = firstVolume->portalPtrs()[index]
                              ->surface()
                              .transform(gctx)
                              .translation();

      auto translationL = lastVolume->portalPtrs()[index]
                              ->surface()
                              .transform(gctx)
                              .translation();

      Vector3 translation = 0.5 * (translationF - stepDown * stepDirection +
                                   translationL + stepUp * stepDirection);

      Transform3 portalTransform = Transform3::Identity();
      portalTransform.prerotate(rotation);
      portalTransform.pretranslate(translation);
      // The half length to be kept
      double keepHalfLength =
          firstBoundValues[toUnderlying(counterPart(mergeValue))];
      mergeParameters[index] = MergeParameters(keepHalfLength, portalTransform);
    }
  }

  // Loop over the volumes and fuse the portals, collect the merge information
  for (auto [iv, v] : enumerate(volumes)) {
    // So far works only in a cubioid setup
    if (v->volumeBounds().type() != VolumeBounds::BoundsType::eCuboid) {
      throw std::invalid_argument(
          "CuboidalDetectorHelper: volume bounds are not cuboid");
    }

    // Loop to fuse the portals along the connection direction (bValue)
    if (iv > 0u) {
      ACTS_VERBOSE("- fuse portals of volume '"
                   << volumes[iv - 1]->name() << "' with volume '" << v->name()
                   << "'.");
      ACTS_VERBOSE("-- indices " << fIndex << " of first,  " << sIndex
                                 << " of second volume.");
      // Fusing the portals of the current volume with the previous one
      auto fPortal = volumes[iv - 1]->portalPtrs()[fIndex];
      auto sPortal = v->portalPtrs()[sIndex];
      auto fusedPortal = Portal::fuse(fPortal, sPortal);
      volumes[iv - 1]->updatePortal(fusedPortal, fIndex);
      v->updatePortal(fusedPortal, sIndex);
    }

    // Get the bound values
    auto boundValues = v->volumeBounds().values();
    // Loop to determine the merge bounds, the new transform
    for (auto [im, mergeValue] : enumerate(possibleValues)) {
      // Skip the bin value itself, fusing will took care of that
      if (mergeValue == bValue) {
        continue;
      }
      // Record the merge splits
      mergeSplits[im].push_back(2 * boundValues[toUnderlying(bValue)]);
      mergeHalfLengths[im] += boundValues[toUnderlying(bValue)];
    }
  }

  // Loop to create the new portals as portal replacements
  std::vector<PortalReplacement> pReplacements;
  for (auto [im, mergeValue] : enumerate(possibleValues)) {
    // Skip the bin value itself, fusing took care of that
    if (mergeValue == bValue) {
      continue;
    }

    // Create the new RectangleBounds
    // - there are conventions involved, regarding the bounds orientation
    // - this is an anticyclic swap
    bool mergedInX = true;
    switch (bValue) {
      case AxisDirection::AxisZ: {
        mergedInX = (mergeValue == AxisDirection::AxisY);
      } break;
      case AxisDirection::AxisY: {
        mergedInX = (mergeValue == AxisDirection::AxisX);
      } break;
      case AxisDirection::AxisX: {
        mergedInX = (mergeValue == AxisDirection::AxisZ);
      } break;
      default:
        break;
    }

    // The stitch boundaries for portal pointing
    std::vector<double> stitchBoundaries;
    stitchBoundaries.push_back(-mergeHalfLengths[im]);
    for (auto step : mergeSplits[im]) {
      stitchBoundaries.push_back(stitchBoundaries.back() + step);
    }

    for (auto [is, index] : enumerate(portalSets[toUnderlying(mergeValue)])) {
      // Check if you need to skip due to selections
      if (!selectedOnly.empty() && !rangeContainsValue(selectedOnly, index)) {
        continue;
      }

      auto [keepHalfLength, portalTransform] = mergeParameters[index];
      std::shared_ptr<RectangleBounds> portalBounds =
          mergedInX ? std::make_shared<RectangleBounds>(mergeHalfLengths[im],
                                                        keepHalfLength)
                    : std::make_shared<RectangleBounds>(keepHalfLength,
                                                        mergeHalfLengths[im]);
      auto portalSurface =
          Surface::makeShared<PlaneSurface>(portalTransform, portalBounds);
      auto portal = std::make_shared<Portal>(portalSurface);

      // Assign the portal direction
      // in a consistent way
      Acts::Direction dir =
          (index % 2 == 0) ? Direction::Forward() : Direction::Backward();

      // Make the stitch boundaries
      pReplacements.push_back(PortalReplacement(
          portal, index, dir, stitchBoundaries,
          (mergedInX ? AxisDirection::AxisX : AxisDirection::AxisY)));
    }
  }

  // Attach the new volume updaters
  PortalHelper::attachExternalNavigationDelegates(gctx, volumes, pReplacements);

  // Return proto container
  DetectorComponent::PortalContainer dShell;

  // Update the portals of all volumes
  // Exchange the portals of the volumes
  for (auto& iv : volumes) {
    ACTS_VERBOSE("- update portals of volume '" << iv->name() << "'.");
    for (auto& [p, i, dir, boundaries, binning] : pReplacements) {
      // Fill the map
      dShell[i] = p;
      ACTS_VERBOSE("-- update portal with index " << i);
      iv->updatePortal(p, i);
    }
  }
  // Done.

  return dShell;
}

Acts::Experimental::DetectorComponent::PortalContainer
Acts::Experimental::detail::CuboidalDetectorHelper::connect(
    const GeometryContext& /*gctx*/,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    AxisDirection bValue, const std::vector<unsigned int>& /*unused */,
    Acts::Logging::Level logLevel) noexcept(false) {
  // The local logger
  ACTS_LOCAL_LOGGER(getDefaultLogger("CuboidalDetectorHelper", logLevel));

  ACTS_DEBUG("Connect " << containers.size() << " containers in "
                        << axisDirectionName(bValue) << ".");

  // Return the new container
  DetectorComponent::PortalContainer dShell;

  // The possible bin values
  std::array<AxisDirection, 3u> possibleValues = {
      AxisDirection::AxisX, AxisDirection::AxisY, AxisDirection::AxisZ};
  // And their associated portal sets, see above
  using PortalSet = std::array<std::size_t, 2u>;
  std::vector<PortalSet> portalSets = {
      {PortalSet{2, 3}, PortalSet{4, 5}, PortalSet{0, 1}}};

  // This is the picked set for refubishing
  auto [endIndex, startIndex] = portalSets[toUnderlying(bValue)];

  // Fusing along the connection direction (bValue)
  for (std::size_t ic = 1; ic < containers.size(); ++ic) {
    auto& formerContainer = containers[ic - 1];
    auto& currentContainer = containers[ic];
    // Check and throw exception
    if (!formerContainer.contains(startIndex)) {
      throw std::invalid_argument(
          "CuboidalDetectorHelper: proto container has no fuse portal at index "
          "of former container.");
    }
    if (!currentContainer.contains(endIndex)) {
      throw std::invalid_argument(
          "CuboidalDetectorHelper: proto container has no fuse portal at index "
          "of current container.");
    }

    std::shared_ptr<Portal> sPortal = formerContainer.find(startIndex)->second;
    auto sAttachedVolumes =
        sPortal->attachedDetectorVolumes()[Direction::Backward().index()];

    std::shared_ptr<Portal> ePortal = currentContainer.find(endIndex)->second;
    auto eAttachedVolumes =
        ePortal->attachedDetectorVolumes()[Direction::Forward().index()];

    auto fusedPortal = Portal::fuse(sPortal, ePortal);

    for (auto& av : sAttachedVolumes) {
      ACTS_VERBOSE("Update portal of detector volume '" << av->name() << "'.");
      av->updatePortal(fusedPortal, startIndex);
    }

    for (auto& av : eAttachedVolumes) {
      ACTS_VERBOSE("Update portal of detector volume '" << av->name() << "'.");
      av->updatePortal(fusedPortal, endIndex);
    }
  }
  // Proto container refurbishment - outside
  dShell[startIndex] = containers.front().find(startIndex)->second;
  dShell[endIndex] = containers.back().find(endIndex)->second;

  // Create remaining outside shells now
  std::vector<unsigned int> sidePortals = {};
  for (auto sVals : possibleValues) {
    if (sVals != bValue) {
      sidePortals.push_back(
          static_cast<unsigned int>(portalSets[toUnderlying(sVals)][0]));
      sidePortals.push_back(
          static_cast<unsigned int>(portalSets[toUnderlying(sVals)][1]));
    }
  }

  // Done.
  return dShell;
}

std::array<std::vector<double>, 3u>
Acts::Experimental::detail::CuboidalDetectorHelper::xyzBoundaries(
    [[maybe_unused]] const GeometryContext& gctx,
    [[maybe_unused]] const std::vector<
        const Acts::Experimental::DetectorVolume*>& volumes,
    Acts::Logging::Level logLevel) {
  // The local logger
  ACTS_LOCAL_LOGGER(getDefaultLogger("CuboidalDetectorHelper", logLevel));

  // The return boundaries
  std::array<std::vector<double>, 3u> boundaries;

  // The map for collecting
  std::array<std::map<double, std::size_t>, 3u> valueMaps;
  auto& xMap = valueMaps[0u];
  auto& yMap = valueMaps[1u];
  auto& zMap = valueMaps[2u];

  auto fillMap = [&](std::map<double, std::size_t>& map,
                     const std::array<double, 2u>& values) {
    for (auto v : values) {
      // This will insert v with a value of 0 if it doesn't exist
      ++map[v];
    }
  };

  // Loop over the volumes and collect boundaries
  for (const auto& v : volumes) {
    if (v->volumeBounds().type() == Acts::VolumeBounds::BoundsType::eCuboid) {
      auto bValues = v->volumeBounds().values();
      // The min/max values
      double halfX = bValues[CuboidVolumeBounds::BoundValues::eHalfLengthX];
      double halfY = bValues[CuboidVolumeBounds::BoundValues::eHalfLengthY];
      double halfZ = bValues[CuboidVolumeBounds::BoundValues::eHalfLengthZ];
      // Get the transform @todo use a center of gravity of the detector
      auto translation = v->transform(gctx).translation();
      // The min/max values
      double xMin = translation.x() - halfX;
      double xMax = translation.x() + halfX;
      double yMin = translation.y() - halfY;
      double yMax = translation.y() + halfY;
      double zMin = translation.z() - halfZ;
      double zMax = translation.z() + halfZ;
      // Fill the maps
      fillMap(xMap, {xMin, xMax});
      fillMap(yMap, {yMin, yMax});
      fillMap(zMap, {zMin, zMax});
    }
  }

  for (auto [im, map] : enumerate(valueMaps)) {
    for (auto [key, _] : map) {
      boundaries[im].push_back(key);
    }
    std::ranges::sort(boundaries[im]);
  }

  ACTS_VERBOSE("- did yield " << boundaries[0u].size() << " boundaries in X.");
  ACTS_VERBOSE("- did yield " << boundaries[1u].size() << " boundaries in Y.");
  ACTS_VERBOSE("- did yield " << boundaries[2u].size() << " boundaries in Z.");

  return boundaries;
}
