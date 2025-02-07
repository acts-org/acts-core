// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/TrackFinding/SeedingOrthogonalAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/EventData/Seed.hpp"
#include "Acts/Seeding/SeedFilter.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"

#include <cmath>
#include <functional>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::SeedingOrthogonalAlgorithm::SeedingOrthogonalAlgorithm(
    ActsExamples::SeedingOrthogonalAlgorithm::Config cfg,
    Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("SeedingAlgorithm", lvl), m_cfg(std::move(cfg)) {
  m_cfg.seedFilterConfig = m_cfg.seedFilterConfig.toInternalUnits();
  m_cfg.seedFinderConfig =
      m_cfg.seedFinderConfig.toInternalUnits().calculateDerivedQuantities();
  m_cfg.seedFinderOptions =
      m_cfg.seedFinderOptions.toInternalUnits().calculateDerivedQuantities(
          m_cfg.seedFinderConfig);

  printOptions();
  printConfig<SimSpacePoint>();

  if (m_cfg.inputSpacePoints.empty()) {
    throw std::invalid_argument("Missing space point input collections");
  }

  for (const auto &spName : m_cfg.inputSpacePoints) {
    if (spName.empty()) {
      throw std::invalid_argument("Invalid space point input collection");
    }

    auto &handle = m_inputSpacePoints.emplace_back(
        std::make_unique<ReadDataHandle<SimSpacePointContainer>>(
            this,
            "InputSpacePoints#" + std::to_string(m_inputSpacePoints.size())));
    handle->initialize(spName);
  }

  if (m_cfg.outputSeeds.empty()) {
    throw std::invalid_argument("Missing seeds output collection");
  }

  m_outputSeeds.initialize(m_cfg.outputSeeds);

  if (m_cfg.seedFilterConfig.maxSeedsPerSpM !=
      m_cfg.seedFinderConfig.maxSeedsPerSpM) {
    throw std::invalid_argument("Inconsistent config maxSeedsPerSpM");
  }

  // construct seed filter
  m_cfg.seedFinderConfig.seedFilter =
      std::make_unique<Acts::SeedFilter<proxy_type>>(
          m_cfg.seedFilterConfig, logger().cloneWithSuffix("Filter"));

  m_finder = std::make_unique<Acts::SeedFinderOrthogonal<proxy_type>>(
      m_cfg.seedFinderConfig, logger().cloneWithSuffix("Finder"));
}

ActsExamples::ProcessCode ActsExamples::SeedingOrthogonalAlgorithm::execute(
    const AlgorithmContext &ctx) const {
  std::vector<const SimSpacePoint *> spacePoints;

  for (const auto &isp : m_inputSpacePoints) {
    for (const auto &spacePoint : (*isp)(ctx)) {
      spacePoints.push_back(&spacePoint);
    }
  }

  // Config
  Acts::SpacePointContainerConfig spConfig;

  // Options
  Acts::SpacePointContainerOptions spOptions;
  spOptions.beamPos = {0., 0.};

  ActsExamples::SpacePointContainer container(spacePoints);
  Acts::SpacePointContainer<decltype(container), Acts::detail::RefHolder>
      spContainer(spConfig, spOptions, container);

  ACTS_INFO("About to process " << spContainer.size() << " space points ...");

  std::vector<Acts::Seed<proxy_type>> seeds =
      m_finder->createSeeds(m_cfg.seedFinderOptions, spContainer);

  ACTS_INFO("Created " << seeds.size() << " track seeds from "
                       << spacePoints.size() << " space points");

  // need to convert here from seed of proxies to seed of sps
  SimSeedContainer seedsToAdd;
  seedsToAdd.reserve(seeds.size());

  for (const auto &seed : seeds) {
    const auto &sps = seed.sp();
    seedsToAdd.emplace_back(*sps[0]->externalSpacePoint(),
                            *sps[1]->externalSpacePoint(),
                            *sps[2]->externalSpacePoint());
    seedsToAdd.back().setVertexZ(seed.z());
    seedsToAdd.back().setQuality(seed.seedQuality());
  }

  m_outputSeeds(ctx, std::move(seedsToAdd));

  return ActsExamples::ProcessCode::SUCCESS;
}

void ActsExamples::SeedingOrthogonalAlgorithm::printOptions() const {
  ACTS_DEBUG("SeedFinderOptions");
  ACTS_DEBUG("beamPos           " << m_cfg.seedFinderOptions.beamPos);
  // field induction
  ACTS_DEBUG("bFieldInZ         " << m_cfg.seedFinderOptions.bFieldInZ);
  // derived quantities
  ACTS_DEBUG("pTPerHelixRadius  " << m_cfg.seedFinderOptions.pTPerHelixRadius);
  ACTS_DEBUG("minHelixDiameter2 " << m_cfg.seedFinderOptions.minHelixDiameter2);
  ACTS_DEBUG("pT2perRadius      " << m_cfg.seedFinderOptions.pT2perRadius);
  ACTS_DEBUG("sigmapT2perRadius " << m_cfg.seedFinderOptions.sigmapT2perRadius);
  ACTS_DEBUG("...\n");
}

template <typename sp>
void ActsExamples::SeedingOrthogonalAlgorithm::printConfig() const {
  ACTS_DEBUG("SeedFinderOrthogonalConfig");
  ACTS_DEBUG("minPt                 " << m_cfg.seedFinderConfig.minPt);
  ACTS_DEBUG("deltaRMinTopSP        " << m_cfg.seedFinderConfig.deltaRMinTopSP);
  ACTS_DEBUG("deltaRMaxTopSP        " << m_cfg.seedFinderConfig.deltaRMaxTopSP);
  ACTS_DEBUG("deltaRMinBottomSP     "
             << m_cfg.seedFinderConfig.deltaRMinBottomSP);
  ACTS_DEBUG("deltaRMaxBottomSP     "
             << m_cfg.seedFinderConfig.deltaRMaxBottomSP);
  ACTS_DEBUG("impactMax             " << m_cfg.seedFinderConfig.impactMax);
  ACTS_DEBUG("maxPtScattering       "
             << m_cfg.seedFinderConfig.maxPtScattering);
  ACTS_DEBUG("collisionRegionMin    "
             << m_cfg.seedFinderConfig.collisionRegionMin);
  ACTS_DEBUG("collisionRegionMax    "
             << m_cfg.seedFinderConfig.collisionRegionMax);
  ACTS_DEBUG("zMin                  " << m_cfg.seedFinderConfig.zMin);
  ACTS_DEBUG("zMax                  " << m_cfg.seedFinderConfig.zMax);
  ACTS_DEBUG("rMax                  " << m_cfg.seedFinderConfig.rMax);
  ACTS_DEBUG("rMin                  " << m_cfg.seedFinderConfig.rMin);
  ACTS_DEBUG("highland              " << m_cfg.seedFinderConfig.highland);
  ACTS_DEBUG("maxScatteringAngle2   "
             << m_cfg.seedFinderConfig.maxScatteringAngle2);
  ACTS_DEBUG("...\n");
}
