// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SpacePointMutableData.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Seeding/CandidatesForMiddleSp.hpp"
#include "Acts/Seeding/Neighbour.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Seeding/SeedFinderUtils.hpp"
#include "Acts/Seeding/SpacePointGrid.hpp"
#include "Acts/Seeding/detail/UtilityFunctions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

template <typename Coll>
concept GridBinCollection =
    std::ranges::random_access_range<Coll> &&
    std::same_as<typename Coll::value_type, std::size_t>;

template <typename collection_t, typename external_t, std::size_t N = 3ul>
concept CollectionStoresSeedsTo =
    requires(collection_t coll, Acts::Seed<external_t, N> seed) {
      Acts::detail::pushBackOrInsertAtEnd(coll, seed);
    };

enum class SpacePointCandidateType : short { eBottom, eTop };

enum class DetectorMeasurementInfo : short { eDefault, eDetailed };

template <typename external_spacepoint_t, typename grid_t,
          typename platform_t = void*>
class SeedFinder {
  ///////////////////////////////////////////////////////////////////
  // Public methods:
  ///////////////////////////////////////////////////////////////////

 public:
  struct SeedingState {
    // bottom space point
    std::vector<const external_spacepoint_t*> compatBottomSP{};
    std::vector<const external_spacepoint_t*> compatTopSP{};
    // contains parameters required to calculate circle with linear equation
    // ...for bottom-middle
    std::vector<LinCircle> linCircleBottom{};
    // ...for middle-top
    std::vector<LinCircle> linCircleTop{};

    // create vectors here to avoid reallocation in each loop
    std::vector<const external_spacepoint_t*> topSpVec{};
    std::vector<float> curvatures{};
    std::vector<float> impactParameters{};

    // managing seed candidates for SpM
    CandidatesForMiddleSp<const external_spacepoint_t> candidates_collector{};

    // managing doublet candidates
    boost::container::small_vector<Acts::Neighbour<grid_t>,
                                   Acts::detail::ipow(3, grid_t::DIM)>
        bottomNeighbours{};
    boost::container::small_vector<Acts::Neighbour<grid_t>,
                                   Acts::detail::ipow(3, grid_t::DIM)>
        topNeighbours{};

    // Mutable variables for Space points used in the seeding
    Acts::SpacePointMutableData spacePointMutableData{};
  };

  /// The only constructor. Requires a config object.
  /// @param config the configuration for the SeedFinder
  /// @param logger the ACTS logger
  explicit SeedFinder(
      const Acts::SeedFinderConfig<external_spacepoint_t>& config,
      std::unique_ptr<const Acts::Logger> logger =
          getDefaultLogger("Finder", Logging::Level::INFO));
  SeedFinder(SeedFinder<external_spacepoint_t, grid_t, platform_t>&&) noexcept =
      default;
  SeedFinder& operator=(SeedFinder<external_spacepoint_t, grid_t,
                                   platform_t>&&) noexcept = default;
  ~SeedFinder() = default;
  /**    @name Disallow default instantiation, copy, assignment */
  //@{
  SeedFinder() = default;
  SeedFinder(const SeedFinder<external_spacepoint_t, grid_t, platform_t>&) =
      delete;
  SeedFinder<external_spacepoint_t, grid_t, platform_t>& operator=(
      const SeedFinder<external_spacepoint_t, grid_t, platform_t>&) = delete;
  //@}

  /// Create all seeds from the space points in the three iterators.
  /// Can be used to parallelize the seed creation
  /// @param options frequently changing configuration (like beam position)
  /// @param state State object that holds memory used
  /// @param grid The grid with space points
  /// @param outputCollection Output container for the seeds in the group
  /// @param bottomSPs group of space points to be used as innermost SP in a seed.
  /// @param middleSPs group of space points to be used as middle SP in a seed.
  /// @param topSPs group of space points to be used as outermost SP in a seed.
  /// @param rMiddleSPRange range object containing the minimum and maximum r for middle SP for a certain z bin.
  /// @note Ranges must return pointers.
  /// @note Ranges must be separate objects for each parallel call.
  template <typename container_t, Acts::GridBinCollection sp_range_t>
    requires Acts::CollectionStoresSeedsTo<container_t, external_spacepoint_t,
                                           3ul>
  void createSeedsForGroup(const Acts::SeedFinderOptions& options,
                           SeedingState& state, const grid_t& grid,
                           container_t& outputCollection,
                           const sp_range_t& bottomSPs,
                           const std::size_t middleSPs,
                           const sp_range_t& topSPs,
                           const Acts::Range1D<float>& rMiddleSPRange) const;

 private:
  /// Given a middle space point candidate, get the proper radius validity range
  /// In case the radius range changes according to the z-bin we need to
  /// retrieve the proper range. We can do this computation only once, since
  /// all the middle space point candidates belong to the same z-bin
  /// @param spM space point candidate to be used as middle SP in a seed
  /// @param rMiddleSPRange range object containing the minimum and maximum r for middle SP for a certain z bin.
  std::pair<float, float> retrieveRadiusRangeForMiddle(
      const external_spacepoint_t& spM,
      const Acts::Range1D<float>& rMiddleSPRange) const;

  /// Iterates over dublets and tests the compatibility between them by applying
  /// a series of cuts that can be tested with only two SPs
  /// @param options frequently changing configuration (like beam position)
  /// @param grid spacepoint grid
  /// @param mutableData Container for mutable variables used in the seeding
  /// @param otherSPsNeighbours inner or outer space points to be used in the dublet
  /// @param mediumSP space point candidate to be used as middle SP in a seed
  /// @param linCircleVec vector containing inner or outer SP parameters after reference frame transformation to the u-v space
  /// @param outVec Output object containing top or bottom SPs that are compatible with a certain middle SPs
  /// @param deltaRMinSP minimum allowed r-distance between dublet components
  /// @param deltaRMaxSP maximum allowed r-distance between dublet components
  /// @param uIP minus one over radius of middle SP
  /// @param uIP2 square of uIP
  /// @param cosPhiM ratio between middle SP x position and radius
  /// @param sinPhiM ratio between middle SP y position and radius
  template <Acts::SpacePointCandidateType candidateType, typename out_range_t>
  void getCompatibleDoublets(
      const Acts::SeedFinderOptions& options, const grid_t& grid,
      Acts::SpacePointMutableData& mutableData,
      boost::container::small_vector<Neighbour<grid_t>,
                                     Acts::detail::ipow(3, grid_t::DIM)>&
          otherSPsNeighbours,
      const external_spacepoint_t& mediumSP,
      std::vector<LinCircle>& linCircleVec, out_range_t& outVec,
      const float deltaRMinSP, const float deltaRMaxSP, const float uIP,
      const float uIP2, const float cosPhiM, const float sinPhiM) const;

  /// Iterates over the seed candidates tests the compatibility between three
  /// SPs and calls for the seed confirmation
  /// @param SpM space point candidate to be used as middle SP in a seed
  /// @param options frequently changing configuration (like beam position)
  /// @param seedFilterState State object that holds memory used in SeedFilter
  /// @param state State object that holds memory used
  template <Acts::DetectorMeasurementInfo detailedMeasurement>
  void filterCandidates(const external_spacepoint_t& SpM,
                        const Acts::SeedFinderOptions& options,
                        SeedFilterState& seedFilterState,
                        SeedingState& state) const;

 private:
  const Logger& logger() const { return *m_logger; }

  Acts::SeedFinderConfig<external_spacepoint_t> m_config;
  std::unique_ptr<const Acts::Logger> m_logger{nullptr};
};

}  // namespace Acts

#ifndef DOXYGEN
#include "Acts/Seeding/SeedFinder.ipp"
#endif
