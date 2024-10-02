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
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <map>
#include <memory>
#include <tuple>
#include <vector>

namespace Acts {
class Direction;

namespace Experimental {

class DetectorVolume;
class Portal;

/// @brief Definition of a portal replacement when building proto containers
///
/// It consists of the new portal, the index, the direction, the parameters
/// gathered from the sub volumes, the binning description
using PortalReplacement =
    std::tuple<std::shared_ptr<Experimental::Portal>, unsigned int, Direction,
               std::vector<ActsScalar>, BinningValue>;

namespace detail::PortalHelper {

/// @brief Method to attach a single detector volume to a portal
///
/// @param portal is the portal where the detector volume is going to be attached
/// @param volume is the volume that is attached to the portal
/// @param direction is the direction to which it is attached
///
void attachExternalNavigationDelegate(
    Portal& portal, const std::shared_ptr<DetectorVolume>& volume,
    const Direction& direction);

/// @brief Create and attach the multi link updator, the portal will get
/// a volume updator attached, that points to the different sub volumes
/// depending on the global position and binning - single assignment case
///
/// @param gctx the geometry context
/// @param portal is the portal where the detector volume is going to be attached
/// @param volumes are the volumes that are pointed to
/// @param direction is the direction to which it is attached
/// @param boundaries are the value boundaries
/// @param binning is the binning type
///
void attachDetectorVolumesUpdater(
    const GeometryContext& gctx, Portal& portal,
    const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    const Direction& direction, const std::vector<ActsScalar>& boundaries,
    const BinningValue& binning);

/// @brief Create and attach the multi link updator, the portal will get
/// a volume updator attached, that points to the different sub volumes
/// depending on the global position and binning
///
/// @param gctx the geometry context
/// @param volumes are the volumes that are pointed to
/// @param pReplacements are the portal replacements that are newly connected
///
void attachExternalNavigationDelegates(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
    std::vector<PortalReplacement>& pReplacements);

/// @brief Method that strips out attached volumes from portals and
/// provides them back to the caller.
///
/// @note it throws an exception if both sides are already taken
///
/// @param portal the portal to be resolved
///
/// @return a vector of attached volumes
std::vector<std::shared_ptr<DetectorVolume>> attachedDetectorVolumes(
    Portal& portal) noexcept(false);

/// @brief Method that strips out attached volumes from portals and
/// provides them back to the caller.
///
/// @param pContainers the portal containers to be resolved
/// @param sides the sides to be handled
/// @param selectedOnly the selected only volumes, e.g. for complex containers
/// to chose only outside skins,
/// @param logLevel the logging level
///
std::map<unsigned int,
         std::vector<std::shared_ptr<Acts::Experimental::DetectorVolume>>>
stripSideVolumes(
    const std::vector<std::map<unsigned int, std::shared_ptr<Portal>>>&
        pContainers,
    const std::vector<unsigned int>& sides,
    const std::vector<unsigned int>& selectedOnly = {},
    Acts::Logging::Level logLevel = Acts::Logging::INFO);

}  // namespace detail::PortalHelper
}  // namespace Experimental
}  // namespace Acts
