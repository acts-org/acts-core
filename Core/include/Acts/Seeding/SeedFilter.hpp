// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/EventData/Seed.hpp"
#include "Acts/EventData/SpacePointMutableData.hpp"
#include "Acts/Seeding/CandidatesForMiddleSp.hpp"
#include "Acts/Seeding/IExperimentCuts.hpp"
#include "Acts/Seeding/SeedFilterConfig.hpp"
#include "Acts/Seeding/detail/UtilityFunctions.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <mutex>
#include <queue>
#include <tuple>
#include <vector>

namespace Acts {
template <typename collection_t, typename external_t, std::size_t N = 3ul>
concept CollectionStoresSeedsTo =
    requires(collection_t coll, Acts::Seed<external_t, N> seed) {
      Acts::detail::pushBackOrInsertAtEnd(coll, seed);
    };

template <typename collection_t, typename external_t, std::size_t N = 3ul>
concept CollectionStoresSeedsToProxied =
    requires(external_t sp) {
      typename external_t::ValueType;
      sp.externalSpacePoint();
    } && CollectionStoresSeedsTo<collection_t,
                                 std::remove_const_t<std::remove_pointer_t<
                                     typename external_t::ValueType> >,
                                 N>;

struct SeedFilterState {
  // longitudinal impact parameter as defined by bottom and middle space point
  float zOrigin = 0;
  // number of minimum top SPs in seed confirmation
  std::size_t nTopSeedConf = 0;
  // radius of bottom component of seed that is used to define the number of
  // compatible top required
  float rMaxSeedConf =
      std::numeric_limits<float>::max();  // Acts::UnitConstants::mm
};

/// Filter seeds at various stages with the currently
/// available information.
template <typename external_spacepoint_t>
class SeedFilter final {
 public:
  SeedFilter(const SeedFilterConfig& config,
             IExperimentCuts<external_spacepoint_t>* expCuts = nullptr);
  SeedFilter(const SeedFilterConfig& config,
             std::unique_ptr<const Acts::Logger> logger,
             IExperimentCuts<external_spacepoint_t>* expCuts = nullptr);
  SeedFilter(const SeedFilter<external_spacepoint_t>&) = delete;
  SeedFilter& operator=(const SeedFilter<external_spacepoint_t>&) = delete;
  SeedFilter(SeedFilter<external_spacepoint_t>&&) noexcept = default;
  SeedFilter& operator=(SeedFilter<external_spacepoint_t>&&) noexcept = default;

  SeedFilter() = delete;
  ~SeedFilter() = default;

  /// Create Seeds for the all seeds with the same bottom and middle
  /// space point and discard all others.
  /// @param mutableData Container for mutable variables used in the seeding
  /// @param bottomSP fixed bottom space point
  /// @param middleSP fixed middle space point
  /// @param topSpVec vector containing all space points that may be compatible
  ///                 with both bottom and middle space point
  /// @param invHelixDiameterVec vector containing 1/(2*r) values where r is the helix radius
  /// @param impactParametersVec vector containing the impact parameters
  /// @param seedFilterState holds quantities used in seed filter
  /// @param candidates_collector container for the seed candidates
  void filterSeeds_2SpFixed(
      const Acts::SpacePointMutableData& mutableData,
      const external_spacepoint_t& bottomSP,
      const external_spacepoint_t& middleSP,
      const std::vector<const external_spacepoint_t*>& topSpVec,
      const std::vector<float>& invHelixDiameterVec,
      const std::vector<float>& impactParametersVec,
      SeedFilterState& seedFilterState,
      CandidatesForMiddleSp<const external_spacepoint_t>& candidates_collector)
      const;

  /// Filter seeds once all seeds for one middle space point have been created
  /// @param mutableData Container for mutable variables used in the seeding
  /// @param candidates_collector collection of seed candidates
  /// @param outputCollection Output container for the seeds
  /// for all seeds with the same middle space point
  template <typename collection_t>
  void filterSeeds_1SpFixed(
      Acts::SpacePointMutableData& mutableData,
      CandidatesForMiddleSp<const external_spacepoint_t>& candidates_collector,
      collection_t& outputCollection) const;

  /// Filter seeds once all seeds for one middle space point have been created
  /// @param mutableData Container for mutable variables used in the seeding
  /// @param candidates collection of seed candidates
  /// @param numQualitySeeds number of high quality seeds in seed confirmation
  /// @param outputCollection Output container for the seeds
  /// for all seeds with the same middle space point
  template <typename collection_t>
  void filterSeeds_1SpFixed(
      Acts::SpacePointMutableData& mutableData,
      std::vector<typename CandidatesForMiddleSp<
          const external_spacepoint_t>::value_type>& candidates,
      const std::size_t numQualitySeeds, collection_t& outputCollection) const;

  const SeedFilterConfig getSeedFilterConfig() const { return m_cfg; }
  const IExperimentCuts<external_spacepoint_t>* getExperimentCuts() const {
    return m_experimentCuts;
  }

 private:
  const Logger& logger() const { return *m_logger; }

  template <typename collection_t>
    requires Acts::CollectionStoresSeedsToProxied<collection_t,
                                                  external_spacepoint_t, 3ul>
  void createAndStoreSeeds(collection_t& outputCollection,
                           const external_spacepoint_t& bottom,
                           const external_spacepoint_t& middle,
                           const external_spacepoint_t& top, float zOrigin,
                           float bestSeedQuality) const;

  const SeedFilterConfig m_cfg;
  std::unique_ptr<const Acts::Logger> m_logger =
      Acts::getDefaultLogger("SeedFilter", Logging::Level::INFO);
  const IExperimentCuts<external_spacepoint_t>* m_experimentCuts;
};
}  // namespace Acts
#include "Acts/Seeding/SeedFilter.ipp"
