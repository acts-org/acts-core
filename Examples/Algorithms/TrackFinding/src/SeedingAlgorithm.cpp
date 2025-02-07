// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFinding/SeedingAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/Seed.hpp"
#include "Acts/Seeding/BinnedGroup.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/GridBinFinder.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"

#include <cmath>
#include <csignal>
#include <cstddef>
#include <limits>
#include <ostream>
#include <stdexcept>

using namespace Acts::HashedStringLiteral;

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::SeedingAlgorithm::SeedingAlgorithm(
    ActsExamples::SeedingAlgorithm::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("SeedingAlgorithm", lvl), m_cfg(std::move(cfg)) {
  using SpacePointProxy_type = typename Acts::SpacePointContainer<
      ActsExamples::SpacePointContainer<std::vector<const SimSpacePoint*>>,
      Acts::detail::RefHolder>::SpacePointProxyType;

  // Seed Finder config requires Seed Filter object before conversion to
  // internal units
  m_cfg.seedFilterConfig = m_cfg.seedFilterConfig.toInternalUnits();
  m_cfg.seedFinderConfig.seedFilter =
      std::make_unique<Acts::SeedFilter<SpacePointProxy_type>>(
          m_cfg.seedFilterConfig, logger().cloneWithSuffix("SeedFilter"));
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

  if (!m_cfg.seedFinderConfig.zBinsCustomLooping.empty()) {
    // check that the bins required in the custom bin looping
    // are contained in the bins defined by the total number of edges

    for (std::size_t i : m_cfg.seedFinderConfig.zBinsCustomLooping) {
      if (i >= m_cfg.gridConfig.zBinEdges.size()) {
        throw std::invalid_argument(
            "Inconsistent config zBinsCustomLooping does not contain a subset "
            "of bins defined by zBinEdges");
      }
    }
  }

  if (m_cfg.useExtraCuts) {
    // This function will be applied to select space points during grid filling
    m_cfg.seedFinderConfig.spacePointSelector
        .connect<itkFastTrackingSPselect>();

    // This function will be applied to the doublet compatibility selection
    m_cfg.seedFinderConfig.experimentCuts.connect<itkFastTrackingCuts>();
  }

  using SpacePointProxy_type = typename Acts::SpacePointContainer<
      ActsExamples::SpacePointContainer<std::vector<const SimSpacePoint*>>,
      Acts::detail::RefHolder>::SpacePointProxyType;

  m_bottomBinFinder = std::make_unique<const Acts::GridBinFinder<3ul>>(
      m_cfg.numPhiNeighbors, cfg.zBinNeighborsBottom, 0);
  m_topBinFinder = std::make_unique<const Acts::GridBinFinder<3ul>>(
      m_cfg.numPhiNeighbors, m_cfg.zBinNeighborsTop, 0);

  m_seedFinder =
      Acts::SeedFinder<SpacePointProxy_type,
                       Acts::CylindricalSpacePointGrid<SpacePointProxy_type>>(
          m_cfg.seedFinderConfig, logger().cloneWithSuffix("SeedFinder"));
}

ActsExamples::ProcessCode ActsExamples::SeedingAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  // construct the combined input container of space point pointers from all
  // configured input sources.
  // pre-compute the total size required so we only need to allocate once
  std::size_t nSpacePoints = 0;
  for (const auto& isp : m_inputSpacePoints) {
    nSpacePoints += (*isp)(ctx).size();
  }

  std::vector<const SimSpacePoint*> spacePointPtrs;
  spacePointPtrs.reserve(nSpacePoints);
  for (const auto& isp : m_inputSpacePoints) {
    for (const auto& spacePoint : (*isp)(ctx)) {
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

  // Prepare interface SpacePoint backend-ACTS
  ActsExamples::SpacePointContainer container(spacePointPtrs);
  // Prepare Acts API
  Acts::SpacePointContainer<decltype(container), Acts::detail::RefHolder>
      spContainer(spConfig, spOptions, container);

  using value_type = typename decltype(spContainer)::SpacePointProxyType;
  using seed_type = Acts::Seed<value_type>;

  Acts::CylindricalSpacePointGrid<value_type> grid =
      Acts::CylindricalSpacePointGridCreator::createGrid<value_type>(
          m_cfg.gridConfig, m_cfg.gridOptions, logger());

  Acts::CylindricalSpacePointGridCreator::fillGrid<value_type>(
      m_cfg.seedFinderConfig, m_cfg.seedFinderOptions, grid, spContainer,
      logger());

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

  auto spacePointsGrouping = Acts::CylindricalBinnedGroup<value_type>(
      std::move(grid), *m_bottomBinFinder, *m_topBinFinder,
      std::move(navigation));

  /// variable middle SP radial region of interest
  const Acts::Range1D<float> rMiddleSPRange(
      std::floor(minRange / 2) * 2 +
          m_cfg.seedFinderConfig.deltaRMiddleMinSPRange,
      std::floor(maxRange / 2) * 2 -
          m_cfg.seedFinderConfig.deltaRMiddleMaxSPRange);

  // run the seeding
  static thread_local std::vector<seed_type> seeds;
  seeds.clear();
  static thread_local decltype(m_seedFinder)::SeedingState state;
  state.spacePointMutableData.resize(spContainer.size());

  for (const auto [bottom, middle, top] : spacePointsGrouping) {
    m_seedFinder.createSeedsForGroup(m_cfg.seedFinderOptions, state,
                                     spacePointsGrouping.grid(), seeds, bottom,
                                     middle, top, rMiddleSPRange);
  }

  ACTS_DEBUG("Created " << seeds.size() << " track seeds from "
                        << spacePointPtrs.size() << " space points");

  // we have seeds of proxies
  // convert them to seed of external space points
  SimSeedContainer SeedContainerForStorage;
  SeedContainerForStorage.reserve(seeds.size());
  for (const auto& seed : seeds) {
    const auto& sps = seed.sp();
    SeedContainerForStorage.emplace_back(*sps[0]->externalSpacePoint(),
                                         *sps[1]->externalSpacePoint(),
                                         *sps[2]->externalSpacePoint());
    SeedContainerForStorage.back().setVertexZ(seed.z());
    SeedContainerForStorage.back().setQuality(seed.seedQuality());
  }

  m_outputSeeds(ctx, std::move(SeedContainerForStorage));
  return ActsExamples::ProcessCode::SUCCESS;
}
