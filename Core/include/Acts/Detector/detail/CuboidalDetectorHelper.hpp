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
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <limits>
#include <map>
#include <memory>
#include <vector>

namespace Acts::Experimental {

class DetectorVolume;
class Portal;

namespace detail::CuboidalDetectorHelper {

/// @brief Connect detector volumes given a binning value
///
/// @param gctx The geometry context
/// @param volumes the volumes
/// @param bValue the binning value  (allowed are binX, binY, binZ)
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note a fair amount of consistency checking is done,
/// and exceptions are thrown if any of the tests fail
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connect(
    const GeometryContext& gctx,
    std::vector<std::shared_ptr<DetectorVolume>>& volumes, BinningValue bValue,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Connect containers given a binning value
///
/// @param gctx The geometry context
/// @param containers the containers
/// @param bValue the binning value  (allowed are binX, binY, binZ)
/// @param selectedOnly switch only selected boundaries
/// @param logLevel is the screen logging level
///
/// @note not much checking is done anymore, as the DetectorComponent::PortalContainer
/// are assumed to come properly formed from the prior methods
///
/// @return a proto container with the outside portals
DetectorComponent::PortalContainer connect(
    const GeometryContext& gctx,
    const std::vector<DetectorComponent::PortalContainer>& containers,
    BinningValue bValue, const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

/// @brief Helper method to extract r,z,phi boundaries for
/// eventual grid volume search
///
/// @param gctx the geometry context of the call
/// @param volumes the volumes at input
/// @param logLevel is the screen logging level
///
/// @return extracted boundary values
std::array<std::vector<ActsScalar>, 3u> xyzBoundaries(
    const GeometryContext& gctx,
    const std::vector<const DetectorVolume*>& volumes,
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

}  // namespace detail::CuboidalDetectorHelper
}  // namespace Acts::Experimental
