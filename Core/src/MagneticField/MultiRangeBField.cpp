// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/MagneticField/MultiRangeBField.hpp"

#include "Acts/MagneticField/MagneticFieldError.hpp"

namespace Acts {

MultiRangeBField::Cache::Cache(const MagneticFieldContext& /*unused*/) {}

MultiRangeBField::MultiRangeBField(const std::vector<BFieldRange>& ranges)
    : fieldRanges(ranges) {}

MultiRangeBField::MultiRangeBField(
    std::vector<MultiRangeBField::BFieldRange>&& ranges)
    : fieldRanges(std::move(ranges)) {}

MagneticFieldProvider::Cache MultiRangeBField::makeCache(
    const MagneticFieldContext& mctx) const {
  return MagneticFieldProvider::Cache(std::in_place_type<Cache>, mctx);
}

Result<Vector3> MultiRangeBField::getField(
    const Vector3& position, MagneticFieldProvider::Cache& cache) const {
  // Because we assume that the regions are in increasing order of
  // precedence, we can iterate over the array, remembering the _last_
  // region that contained the given point. At the end of the loop, this
  // region will then be the one we query for its field vector.
  std::optional<std::size_t> foundRange = {};

  // The cache object for this type contains an optional integer; if the
  // integer is set, it indicates the index of the region in which the last
  // access succeeded. This acts as a cache because if the current access is
  // still within that region, it is guaranteed that none of the regions
  // with lower priority -- i.e. that are earlier in the region vector --
  // can be relevant to the current access. Thus, we request the cache index
  // if it exists and perform a membership check on it; if that succeeds, we
  // remember the corresponding region as a candidate.
  if (Cache& lCache = cache.as<Cache>();
      lCache.index.has_value() &&
      std::get<0>(fieldRanges[*lCache.index])
          .contains({position[0], position[1], position[2]})) {
    foundRange = *lCache.index;
  }

  // Now, we iterate over the ranges. If we already have a range candidate,
  // we start iteration just after the existing candidate; otherwise we
  // start from the beginning.
  for (std::size_t i = (foundRange.has_value() ? (*foundRange) + 1 : 0);
       i < fieldRanges.size(); ++i) {
    if (std::get<0>(fieldRanges[i])
            .contains({position[0], position[1], position[2]})) {
      foundRange = i;
    }
  }

  // Update the cache using the result of this access.
  cache.as<Cache>().index = foundRange;

  // If we found a valid range, return the corresponding vector; otherwise
  // return an out-of-bounds error.
  if (foundRange.has_value()) {
    return Result<Vector3>::success(std::get<1>(fieldRanges[*foundRange]));
  } else {
    return Result<Vector3>::failure(MagneticFieldError::OutOfBounds);
  }
}

}  // namespace Acts
