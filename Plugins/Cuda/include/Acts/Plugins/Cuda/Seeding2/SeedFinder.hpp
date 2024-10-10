// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Seeding2/TripletFilterConfig.hpp"

// Acts include(s).
#include "Acts/EventData/SpacePointData.hpp"
#include "Acts/Seeding/Seed.hpp"
#include "Acts/Seeding/SeedFilterConfig.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Seeding/SpacePointGrid.hpp"
#include "Acts/Utilities/Logger.hpp"

// System include(s).
#include <memory>

namespace Acts {
namespace Cuda {

template <typename external_spacepoint_t>
class SeedFinder {
  ///////////////////////////////////////////////////////////////////
  // Public methods:
  ///////////////////////////////////////////////////////////////////

 public:
  /// Create a CUDA backed seed finder object
  ///
  /// @param commonConfig Configuration shared with @c Acts::SeedFinder
  /// @param seedFinderOptions options also shared with Acts::SeedFinder
  /// @param seedFilterConfig Configuration shared with @c Acts::SeedFilter
  /// @param tripletFilterConfig Configuration for the GPU based triplet
  ///        filtering
  /// @param device The identifier of the CUDA device to run on
  /// @param logger A @c Logger instance
  ///
  SeedFinder(SeedFinderConfig<external_spacepoint_t> commonConfig,
             const SeedFinderOptions& seedFinderOptions,
             const SeedFilterConfig& seedFilterConfig,
             const TripletFilterConfig& tripletFilterConfig, int device = 0,
             std::unique_ptr<const Logger> logger =
                 getDefaultLogger("Cuda::SeedFinder", Logging::INFO));

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

  /// set logging instance
  ///
  /// @param [in] newLogger is the logging instance to be set
  void setLogger(std::unique_ptr<const Logger> newLogger);

 private:
  /// Private access to the logger
  ///
  /// @return a const reference to the logger
  const Logger& logger() const { return *m_logger; }

  /// Configuration for the seed finder
  SeedFinderConfig<external_spacepoint_t> m_commonConfig;
  SeedFinderOptions m_seedFinderOptions;
  /// Configuration for the (host) seed filter
  SeedFilterConfig m_seedFilterConfig;
  /// Configuration for the (device) triplet filter
  TripletFilterConfig m_tripletFilterConfig;
  /// CUDA device identifier
  int m_device;
  /// The logger object
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Cuda
}  // namespace Acts

// Include the template implementation.
#include "Acts/Plugins/Cuda/Seeding2/SeedFinder.ipp"
