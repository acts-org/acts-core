// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/SeedingAlgorithmHashing.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/Seed.hpp"
#include "Acts/EventData/SpacePointData.hpp"
#include "Acts/Plugins/Hashing/HashingAlgorithm.hpp"
#include "Acts/Plugins/Hashing/HashingTraining.hpp"
#include "Acts/Seeding/BinnedGroup.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Seeding/SeedFinder.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridBinFinder.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <cmath>
#include <csignal>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::SeedingAlgorithmHashing::SeedingAlgorithmHashing(
    ActsExamples::SeedingAlgorithmHashing::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("SeedingAlgorithmHashing", lvl),
      m_cfg(std::move(cfg)) {
  using SpacePointProxy_type = typename Acts::SpacePointContainer<
      ActsExamples::SpacePointContainer<std::vector<const SimSpacePoint*>>,
      Acts::detail::RefHolder>::SpacePointProxyType;

  // Seed Finder config requires Seed Filter object before conversion to
  // internal units
  m_cfg.seedFilterConfig = m_cfg.seedFilterConfig.toInternalUnits();
  m_cfg.seedFinderConfig.seedFilter =
      std::make_unique<Acts::SeedFilter<SpacePointProxy_type>>(
          m_cfg.seedFilterConfig);

  m_cfg.seedFinderConfig =
      m_cfg.seedFinderConfig.toInternalUnits().calculateDerivedQuantities();
  m_cfg.seedFinderOptions =
      m_cfg.seedFinderOptions.toInternalUnits().calculateDerivedQuantities(
          m_cfg.seedFinderConfig);
  m_cfg.gridConfig = m_cfg.gridConfig.toInternalUnits();
  m_cfg.gridOptions = m_cfg.gridOptions.toInternalUnits();
  if (m_cfg.inputSpacePoints.empty()) {
    throw std::invalid_argument("Missing space point input collections");
  }

  for (const auto& spName : m_cfg.inputSpacePoints) {
    if (spName.empty()) {
      throw std::invalid_argument("Invalid space point input collection");
    }

    auto& handle = m_inputSpacePoints.emplace_back(
        std::make_unique<ReadDataHandle<SimSpacePointContainer>>(
            this,
            "InputSpacePoints#" + std::to_string(m_inputSpacePoints.size())));
    handle->initialize(spName);
  }
  if (m_cfg.outputSeeds.empty()) {
    throw std::invalid_argument("Missing seeds output collection");
  }

  m_outputSeeds.initialize(m_cfg.outputSeeds);
  m_outputBuckets.initialize(m_cfg.outputBuckets);

  if (m_cfg.gridConfig.rMax != m_cfg.seedFinderConfig.rMax &&
      m_cfg.allowSeparateRMax == false) {
    throw std::invalid_argument(
        "Inconsistent config rMax: using different values in gridConfig and "
        "seedFinderConfig. If values are intentional set allowSeparateRMax to "
        "true");
  }

  if (m_cfg.seedFilterConfig.deltaRMin != m_cfg.seedFinderConfig.deltaRMin) {
    throw std::invalid_argument("Inconsistent config deltaRMin");
  }

  if (m_cfg.gridConfig.deltaRMax != m_cfg.seedFinderConfig.deltaRMax) {
    throw std::invalid_argument("Inconsistent config deltaRMax");
  }

  static_assert(
      std::numeric_limits<
          decltype(m_cfg.seedFinderConfig.deltaRMaxTopSP)>::has_quiet_NaN,
      "Value of deltaRMaxTopSP must support NaN values");

  static_assert(
      std::numeric_limits<
          decltype(m_cfg.seedFinderConfig.deltaRMinTopSP)>::has_quiet_NaN,
      "Value of deltaRMinTopSP must support NaN values");

  static_assert(
      std::numeric_limits<
          decltype(m_cfg.seedFinderConfig.deltaRMaxBottomSP)>::has_quiet_NaN,
      "Value of deltaRMaxBottomSP must support NaN values");

  static_assert(
      std::numeric_limits<
          decltype(m_cfg.seedFinderConfig.deltaRMinBottomSP)>::has_quiet_NaN,
      "Value of deltaRMinBottomSP must support NaN values");

  if (std::isnan(m_cfg.seedFinderConfig.deltaRMaxTopSP)) {
    m_cfg.seedFinderConfig.deltaRMaxTopSP = m_cfg.seedFinderConfig.deltaRMax;
  }

  if (std::isnan(m_cfg.seedFinderConfig.deltaRMinTopSP)) {
    m_cfg.seedFinderConfig.deltaRMinTopSP = m_cfg.seedFinderConfig.deltaRMin;
  }

  if (std::isnan(m_cfg.seedFinderConfig.deltaRMaxBottomSP)) {
    m_cfg.seedFinderConfig.deltaRMaxBottomSP = m_cfg.seedFinderConfig.deltaRMax;
  }

  if (std::isnan(m_cfg.seedFinderConfig.deltaRMinBottomSP)) {
    m_cfg.seedFinderConfig.deltaRMinBottomSP = m_cfg.seedFinderConfig.deltaRMin;
  }

  if (m_cfg.gridConfig.zMin != m_cfg.seedFinderConfig.zMin) {
    throw std::invalid_argument("Inconsistent config zMin");
  }

  if (m_cfg.gridConfig.zMax != m_cfg.seedFinderConfig.zMax) {
    throw std::invalid_argument("Inconsistent config zMax");
  }

  if (m_cfg.seedFilterConfig.maxSeedsPerSpM !=
      m_cfg.seedFinderConfig.maxSeedsPerSpM) {
    throw std::invalid_argument("Inconsistent config maxSeedsPerSpM");
  }

  if (m_cfg.gridConfig.cotThetaMax != m_cfg.seedFinderConfig.cotThetaMax) {
    throw std::invalid_argument("Inconsistent config cotThetaMax");
  }

  if (m_cfg.gridConfig.minPt != m_cfg.seedFinderConfig.minPt) {
    throw std::invalid_argument("Inconsistent config minPt");
  }

  if (m_cfg.gridOptions.bFieldInZ != m_cfg.seedFinderOptions.bFieldInZ) {
    throw std::invalid_argument("Inconsistent config bFieldInZ");
  }

  if (m_cfg.gridConfig.zBinEdges.size() - 1 != m_cfg.zBinNeighborsTop.size() &&
      m_cfg.zBinNeighborsTop.empty() == false) {
    throw std::invalid_argument("Inconsistent config zBinNeighborsTop");
  }

  if (m_cfg.gridConfig.zBinEdges.size() - 1 !=
          m_cfg.zBinNeighborsBottom.size() &&
      m_cfg.zBinNeighborsBottom.empty() == false) {
    throw std::invalid_argument("Inconsistent config zBinNeighborsBottom");
  }

  if (m_cfg.useExtraCuts) {
    // This function will be applied to select space points during grid filling
    m_cfg.seedFinderConfig.spacePointSelector
        .connect<itkFastTrackingSPselect>();

    // This function will be applied to the doublet compatibility selection
    m_cfg.seedFinderConfig.experimentCuts.connect<itkFastTrackingCuts>();
  }

  m_bottomBinFinder = std::make_unique<const Acts::GridBinFinder<3ul>>(
      m_cfg.numPhiNeighbors, m_cfg.zBinNeighborsBottom, 0);
  m_topBinFinder = std::make_unique<const Acts::GridBinFinder<3ul>>(
      m_cfg.numPhiNeighbors, m_cfg.zBinNeighborsTop, 0);

  m_cfg.seedFinderConfig.seedFilter =
      std::make_unique<Acts::SeedFilter<SpacePointProxy_type>>(
          m_cfg.seedFilterConfig);
  m_seedFinder =
      Acts::SeedFinder<SpacePointProxy_type,
                       Acts::CylindricalSpacePointGrid<SpacePointProxy_type>>(
          m_cfg.seedFinderConfig);
  m_hashing = Acts::HashingAlgorithm<const SimSpacePoint*,
                                     std::vector<const SimSpacePoint*>>(
      m_cfg.hashingConfig);
  m_hashingTraining =
      Acts::HashingTrainingAlgorithm<std::vector<const SimSpacePoint*>>(
          m_cfg.hashingTrainingConfig);
}

ActsExamples::ProcessCode ActsExamples::SeedingAlgorithmHashing::execute(
    const AlgorithmContext& ctx) const {
  ACTS_DEBUG("Start of SeedingAlgorithmHashing execute");
  using SpacePointPtrVector = std::vector<const SimSpacePoint*>;

  // construct the combined input container of space point pointers from all
  // configured input sources.
  // pre-compute the total size required so we only need to allocate once
  std::size_t nSpacePoints = 0;
  for (const auto& isp : m_inputSpacePoints) {
    nSpacePoints += (*isp)(ctx).size();
  }

  SpacePointPtrVector spacePointPtrs;
  spacePointPtrs.reserve(nSpacePoints);
  for (const auto& isp : m_inputSpacePoints) {
    for (const SimSpacePoint& spacePoint : (*isp)(ctx)) {
      // since the event store owns the space
      // points, their pointers should be stable and
      // we do not need to create local copies.
      spacePointPtrs.push_back(&spacePoint);
    }
  }

  // Config
  Acts::SpacePointContainerConfig spConfig;
  spConfig.useDetailedDoubleMeasurementInfo =
      m_cfg.seedFinderConfig.useDetailedDoubleMeasurementInfo;
  // Options
  Acts::SpacePointContainerOptions spOptions;
  spOptions.beamPos = {0., 0.};

  // Hashing Training
  Acts::AnnoyModel annoyModel = m_hashingTraining.execute(spacePointPtrs);
  // Hashing
  static thread_local std::vector<SpacePointPtrVector> bucketsPtrs;
  bucketsPtrs.clear();
  m_hashing.execute(spacePointPtrs, &annoyModel, bucketsPtrs);

  // pre-compute the maximum size required so we only need to allocate once
  // doesn't combine the input containers of space point pointers
  std::size_t maxNSpacePoints = 0;
  std::size_t inSpacePoints = 0;
  for (const SpacePointPtrVector& bucket : bucketsPtrs) {
    inSpacePoints = bucket.size();
    if (inSpacePoints > maxNSpacePoints) {
      maxNSpacePoints = inSpacePoints;
    }
  }

  using value_type = typename Acts::SpacePointContainer<
      ActsExamples::SpacePointContainer<std::vector<const SimSpacePoint*>>,
      Acts::detail::RefHolder>::SpacePointProxyType;
  using seed_type = Acts::Seed<value_type>;

  // Create the set with custom comparison function
  static thread_local std::set<ActsExamples::SimSeed,
                               SeedComparison<SimSpacePoint>>
      seedsSet;
  seedsSet.clear();
  static thread_local decltype(m_seedFinder)::SeedingState state;
  state.spacePointMutableData.resize(maxNSpacePoints);

  for (SpacePointPtrVector& bucket : bucketsPtrs) {
    std::set<seed_type, SeedComparison<value_type>> seedsSetForBucket;
    state.spacePointMutableData.clear();
    state.spacePointMutableData.resize(maxNSpacePoints);

    // Prepare interface SpacePoint backend-ACTS
    ActsExamples::SpacePointContainer<SpacePointPtrVector> container(bucket);
    // Prepare Acts API
    Acts::SpacePointContainer<decltype(container), Acts::detail::RefHolder>
        spContainer(spConfig, spOptions, container);

    // construct the seeding tools
    Acts::CylindricalSpacePointGrid<value_type> grid =
        Acts::CylindricalSpacePointGridCreator::createGrid<value_type>(
            m_cfg.gridConfig, m_cfg.gridOptions);
    Acts::CylindricalSpacePointGridCreator::fillGrid(
        m_cfg.seedFinderConfig, m_cfg.seedFinderOptions, grid,
        spContainer.begin(), spContainer.end());

    // Compute radius Range
    // we rely on the fact the grid is storing the proxies
    // with a sorting in the radius
    float minRange = std::numeric_limits<float>::max();
    float maxRange = std::numeric_limits<float>::lowest();
    for (const auto& coll : grid) {
      if (coll.empty()) {
        continue;
      }
      const auto* firstEl = coll.front();
      const auto* lastEl = coll.back();
      minRange = std::min(firstEl->radius(), minRange);
      maxRange = std::max(lastEl->radius(), maxRange);
    }

    std::array<std::vector<std::size_t>, 3ul> navigation;
    navigation[1ul] = m_cfg.seedFinderConfig.zBinsCustomLooping;

    // groups spacepoints
    auto spacePointsGrouping = Acts::CylindricalBinnedGroup<value_type>(
        std::move(grid), *m_bottomBinFinder, *m_topBinFinder,
        std::move(navigation));

    /// variable middle SP radial region of interest
    const Acts::Range1D<float> rMiddleSPRange(
        minRange + m_cfg.seedFinderConfig.deltaRMiddleMinSPRange,
        maxRange - m_cfg.seedFinderConfig.deltaRMiddleMaxSPRange);

    // this creates seeds of proxy, we need to convert it to seed of space
    // points
    for (const auto [bottom, middle, top] : spacePointsGrouping) {
      m_seedFinder.createSeedsForGroup(
          m_cfg.seedFinderOptions, state, spacePointsGrouping.grid(),
          seedsSetForBucket, bottom, middle, top, rMiddleSPRange);
    }

    // proxies die when the Acts::SpacePointContainer dies, so we need to
    // convert the seeds of space points proxies to seeds of space points
    for (const seed_type& seed : seedsSetForBucket) {
      const std::array<const value_type*, 3>& sps = seed.sp();
      const SimSpacePoint* bottom = sps[0]->externalSpacePoint();
      const SimSpacePoint* middle = sps[1]->externalSpacePoint();
      const SimSpacePoint* top = sps[2]->externalSpacePoint();

      ActsExamples::SimSeed toAdd(*bottom, *middle, *top);
      toAdd.setVertexZ(seed.z());
      toAdd.setQuality(seed.seedQuality());

      seedsSet.insert(toAdd);
    }
  }

  // convert the set to a simseed collection
  ActsExamples::SimSeedContainer seeds;
  seeds.reserve(seedsSet.size());
  for (const ActsExamples::SimSeed& seed : seedsSet) {
    seeds.push_back(seed);
  }

  ACTS_INFO("Created " << seeds.size() << " track seeds from "
                       << spacePointPtrs.size() << " space points");

  m_outputSeeds(ctx, SimSeedContainer{seeds});
  std::vector<SimSpacePointContainer> buckets;
  for (const SpacePointPtrVector& bucket : bucketsPtrs) {
    SimSpacePointContainer bucketSP;
    for (const SimSpacePoint* spacePoint : bucket) {
      bucketSP.push_back(*spacePoint);
    }
    buckets.push_back(bucketSP);
  }
  m_outputBuckets(ctx, std::vector<SimSpacePointContainer>{buckets});

  ACTS_DEBUG("End of SeedingAlgorithmHashing execute");
  return ActsExamples::ProcessCode::SUCCESS;
}
