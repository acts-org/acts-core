// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <limits>
#include <map>
#include <memory>
#include <vector>

namespace Acts::Experimental {

class DetectorVolume;
class Portal;

namespace detail::CylindricalDetectorHelper {

/// @brief Connect detector volumes in R
///
/// @param gctx The geometry context
/// @param volumes the volumes
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note a fair amount of consistency checking is done,
/// and exceptions are thrown if any of the tests fail
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connectInR(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect detector volumes in Z
///
/// @param gctx The geometry context
/// @param volumes the volumes
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note a fair amount of consistency checking is done,
/// and exceptions are thrown if any of the tests fail
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connectInZ(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect detector volumes in phi
///
/// @param gctx The geometry context
/// @param volumes the volumes
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note a fair amount of consistency checking is done,
/// and exceptions are thrown if any of the tests fail
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connectInPhi(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Wrap detector volumes in R,Z
///
/// @param gctx The geometry context
/// @param volumes the volumes
/// @param logLevel is the screen logging level
///
/// @note a fair amount of consistency checking is done,
/// and exceptions are thrown if any of the tests fail
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer wrapInZR(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect containers in R
///
/// @param gctx The geometry context
/// @param containers the containers
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note not much checking is done anymore, as the DetectorComponent::PortalContainer
/// are assumed to come properly formed from the prior methods
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connectInR(
    const GeometryContext& gctx,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect containers in Z
///
/// @param gctx The geometry context
/// @param containers the containers
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note not much checking is done anymore, as the DetectorComponent::PortalContainer
/// are assumed to come properly formed from the prior methods
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connectInZ(
    const GeometryContext& gctx,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect containers in Phi
///
/// @param gctx The geometry context
/// @param containers the containers
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note not much checking is done anymore, as the DetectorComponent::PortalContainer
/// are assumed to come properly formed from the prior methods
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connectInPhi(
    const GeometryContext& gctx,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Wrap container in R,Z - this uses the cutout cylinder bounds
///
/// @param gctx The geometry context
/// @param containers the containers, i.e. the inner volume and the wrapping container
/// @param logLevel is the screen logging level
///
/// @note not much checking is done anymore, as the DetectorComponent::PortalContainer
/// are assumed to come properly formed from the prior methods
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer wrapInZR(
    const GeometryContext& gctx,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Helper method to extract r,z,phi boundaries for
/// eventual grid volume search
///
/// @tparam volume_container_t the type of the container
///
/// @param gctx the geometry context of the call
/// @param volumes the volumes at input
/// @param precision the precision to be used for (optionally)
/// @param logLevel is the screen logging level
///
/// @return extracted boundary values
template <typename volume_container_t>
std::array<std::vector<double>, 3u> rzphiBoundaries(
    const GeometryContext& gctx, const volume_container_t& volumes,
    double precision = 0.,
    Acts::Logging::Level logLevel = Acts::Logging::INFO) {
  // The local logger
  ACTS_LOCAL_LOGGER(getDefaultLogger("CylindricalDetectorHelper", logLevel));

  ACTS_DEBUG("Estimate R/z/phi boundaries of  " << volumes.size()
                                                << " volumes.");

  // The return boundaries
  std::array<std::set<double>, 3u> uniqueBoundaries;
  auto insertWithPrecision = [&](std::size_t is, double value) -> void {
    if (precision == 0.) {
      uniqueBoundaries[is].insert(value);
      return;
    }
    uniqueBoundaries[is].insert(std::round(value / precision) * precision);
  };

  // Loop over the volumes and collect boundaries
  for (const auto& v : volumes) {
    if (v->volumeBounds().type() == VolumeBounds::BoundsType::eCylinder) {
      const auto& bValues = v->volumeBounds().values();
      // The min/max values
      double rMin = bValues[CylinderVolumeBounds::BoundValues::eMinR];
      double rMax = bValues[CylinderVolumeBounds::BoundValues::eMaxR];
      double zCenter = v->transform(gctx).translation().z();
      double zHalfLength =
          bValues[CylinderVolumeBounds::BoundValues::eHalfLengthZ];
      double zMin = zCenter - zHalfLength;
      double zMax = zCenter + zHalfLength;
      double phiCenter =
          bValues[CylinderVolumeBounds::BoundValues::eAveragePhi];
      double phiSector =
          bValues[CylinderVolumeBounds::BoundValues::eHalfPhiSector];
      double phiMin = phiCenter - phiSector;
      double phiMax = phiCenter + phiSector;
      // Fill the sets
      insertWithPrecision(0u, rMin);
      insertWithPrecision(0u, rMax);
      insertWithPrecision(1u, zMin);
      insertWithPrecision(1u, zMax);
      insertWithPrecision(2u, phiMin);
      insertWithPrecision(2u, phiMax);
    }
  }

  ACTS_VERBOSE("- did yield " << uniqueBoundaries[0u].size()
                              << " boundaries in R.");
  ACTS_VERBOSE("- did yield " << uniqueBoundaries[1u].size()
                              << " boundaries in z.");
  ACTS_VERBOSE("- did yield " << uniqueBoundaries[2u].size()
                              << " boundaries in phi.");

  return {{std::vector<double>(uniqueBoundaries[0].begin(),
                               uniqueBoundaries[0].end()),
           std::vector<double>(uniqueBoundaries[1].begin(),
                               uniqueBoundaries[1].end()),
           std::vector<double>(uniqueBoundaries[2].begin(),
                               uniqueBoundaries[2].end())}};
}

}  // namespace detail::CylindricalDetectorHelper
}  // namespace Acts::Experimental
