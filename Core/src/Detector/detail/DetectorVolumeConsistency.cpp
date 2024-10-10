// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/detail/DetectorVolumeConsistency.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Utilities/StringHelpers.hpp"

#include <stdexcept>

void Acts::Experimental::detail::DetectorVolumeConsistency::
    checkRotationAlignment(
        const GeometryContext& gctx,
        const std::vector<std::shared_ptr<Experimental::DetectorVolume>>&
            volumes) {
  // Take first transform as reference transform
  auto refRotation = volumes[0u]->transform(gctx).rotation();
  // Loop over rest and recursively test
  for (auto [iv, v] : Acts::enumerate(volumes)) {
    if (iv > 0) {
      auto curRotation = v->transform(gctx).rotation();
      if (!curRotation.isApprox(refRotation)) {
        std::string message = "ConsitencyChecker: rotation of volume ";
        message += std::to_string(iv);
        message += std::string(" is not aligned with previous volume");
        throw std::invalid_argument(message.c_str());
      }
    }
  }
}

std::vector<Acts::ActsScalar>
Acts::Experimental::detail::DetectorVolumeConsistency::checkCenterAlignment(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<Experimental::DetectorVolume>>& volumes,
    BinningValue axisValue) {
  std::vector<ActsScalar> distances = {};
  // First it needs to surfive the rotation check
  checkRotationAlignment(gctx, volumes);

  // Get the reference axis
  Vector3 refAxis =
      volumes[0u]->transform(gctx).rotation().col(toUnderlying(axisValue));

  for (auto [iv, v] : enumerate(volumes)) {
    if (iv > 0) {
      Vector3 lastCenter = volumes[iv - 1]->transform(gctx).translation();
      Vector3 curCenter = v->transform(gctx).translation();
      Vector3 diff(curCenter - lastCenter);
      // Check if the difference is aligned with the reference axis
      if (!diff.normalized().isApprox(refAxis)) {
        std::string message = "ConsitencyChecker: center ";
        message += toString(curCenter);
        message += " of volume ";
        message += std::to_string(iv);
        message += " is not aligned with center ";
        message += toString(lastCenter);
        message += " of previous volume.";
        message += " Axis mismatch:  ";
        message += toString(refAxis);
        message += " vs. ";
        message += toString(diff.normalized());
        throw std::invalid_argument(message.c_str());
      }
      // Check if the projection is positive
      if (diff.dot(refAxis) < 0.) {
        std::string message = "ConsitencyChecker: center of volume ";
        message += std::to_string(iv);
        message += std::string(" is not ordered with previous volume");
        throw std::invalid_argument(message.c_str());
      }
      distances.push_back(diff.norm());
    }
  }
  return distances;
}
