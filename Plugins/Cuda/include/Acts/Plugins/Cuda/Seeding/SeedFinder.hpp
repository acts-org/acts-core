// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Cuda/Cuda.hpp"
#include "Acts/Plugins/Cuda/Seeding/Kernels.cuh"
#include "Acts/Seeding/InternalSeed.hpp"
#include "Acts/Seeding/InternalSpacePoint.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedFinder.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Seeding/SpacePointGrid.hpp"

#include <array>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

template <typename external_spacepoint_t>
class SeedFinder<external_spacepoint_t, Acts::Cuda> {
  ///////////////////////////////////////////////////////////////////
  // Public methods:
  ///////////////////////////////////////////////////////////////////

 public:
  SeedFinder(const Acts::SeedFinderConfig<external_spacepoint_t>& config,
             const Acts::SeedFinderOptions& options);

  ~SeedFinder() = default;
  /**    @name Disallow default instantiation, copy, assignment */
  //@{
  SeedFinder() = delete;
  SeedFinder(const SeedFinder<external_spacepoint_t, Acts::Cuda>&) = delete;
  SeedFinder<external_spacepoint_t, Acts::Cuda>& operator=(
      const SeedFinder<external_spacepoint_t, Acts::Cuda>&) = delete;
  //@}

  /// Create all seeds from the space points in the three iterators.
  /// Can be used to parallelize the seed creation
  /// @param bottomSPs group of space points to be used as innermost SP in a seed.
  /// @param middleSPs group of space points to be used as middle SP in a seed.
  /// @param topSPs group of space points to be used as outermost SP in a seed.
  /// Ranges must return pointers.
  /// Ranges must be separate objects for each parallel call.
  /// @return vector in which all found seeds for this group are stored.
  template <typename sp_range_t>
  std::vector<Seed<external_spacepoint_t> > createSeedsForGroup(
      Acts::SpacePointData& spacePointData,
      Acts::CylindricalSpacePointGrid<external_spacepoint_t>& grid,
      const sp_range_t& bottomSPs, const std::size_t middleSPs,
      const sp_range_t& topSPs) const;

 private:
  Acts::SeedFinderConfig<external_spacepoint_t> m_config;
  Acts::SeedFinderOptions m_options;
};

}  // namespace Acts

#include "Acts/Plugins/Cuda/Seeding/SeedFinder.ipp"
