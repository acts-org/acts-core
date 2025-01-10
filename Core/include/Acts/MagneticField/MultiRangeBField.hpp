// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/RangeXD.hpp"

namespace Acts {

/// @ingroup MagneticField
///
/// @brief Magnetic field provider modelling a magnetic field consisting of
/// several (potentially overlapping) regions of constant values.
class MultiRangeBField final : public MagneticFieldProvider {
 private:
  struct Cache {
    explicit Cache(const MagneticFieldContext& /*unused*/);

    std::optional<std::size_t> index = {};
  };

  using BFieldRange = std::pair<RangeXD<3, double>, Vector3>;

  // The different ranges and their corresponding field vectors. Note that
  // regions positioned _later_ in this vector take priority over earlier
  // regions.
  std::vector<BFieldRange> fieldRanges;

 public:
  /// @brief Construct a magnetic field from a vector of ranges.
  ///
  /// @warning These ranges are listed in increasing order of precedence,
  /// i.e. ranges further along the vector have higher priority.
  explicit MultiRangeBField(const std::vector<BFieldRange>& ranges);

  explicit MultiRangeBField(std::vector<BFieldRange>&& ranges);

  /// @brief Construct a cache object.
  MagneticFieldProvider::Cache makeCache(
      const MagneticFieldContext& mctx) const override;

  /// @brief Request the value of the magnetic field at a given position.
  ///
  /// @param [in] position Global 3D position for the lookup.
  /// @param [in, out] cache Cache object.
  /// @returns A successful value containing a field vector if the given
  /// location is contained inside any of the regions, or a failure value
  /// otherwise.
  Result<Vector3> getField(const Vector3& position,
                           MagneticFieldProvider::Cache& cache) const override;
};

}  // namespace Acts
