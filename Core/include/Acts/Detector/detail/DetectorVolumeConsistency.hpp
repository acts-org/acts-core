// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Common.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinningData.hpp"

#include <memory>
#include <vector>

namespace Acts::Experimental {

class DetectorVolume;

namespace detail::DetectorVolumeConsistency {

/// @brief Helper method to check alignment of the volumes, this method checks
/// if the rotational part of the transform is identical
///
/// @param gctx the geometry context
/// @param volumes the input volumes to be checked
///
/// @note this is a strict matching that requires the rotation to be identical
///
/// @note throws exception if any of checks fails
void checkRotationAlignment(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<Experimental::DetectorVolume>>& volumes);

/// @brief Helper method to check whether a set of volumes is lined up on
/// a given common axis definition
///
/// @param gctx the geometry context
/// @param volumes the input volumes to be checked
/// @param axisValue the alignment axist
///
/// @note this will call checkRotationAlignment first
/// @note throws exception if the volumes are not ordered
///
/// @return a vector with position differences (ordered)
std::vector<ActsScalar> checkCenterAlignment(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<Experimental::DetectorVolume>>& volumes,
    BinningValue axisValue);

}  // namespace detail::DetectorVolumeConsistency
}  // namespace Acts::Experimental
