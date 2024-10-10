// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/DigitizationAlgorithm.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/Digitization/ModuleClusters.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Utilities/GroupBy.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <ostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

ActsExamples::DigitizationAlgorithm::DigitizationAlgorithm(
    DigitizationConfig config, Acts::Logging::Level level)
    : ActsExamples::IAlgorithm("DigitizationAlgorithm", level),
      m_cfg(std::move(config)) {
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }
  if (m_cfg.surfaceByIdentifier.empty()) {
    throw std::invalid_argument("Missing Surface-GeometryID association map");
  }
  if (!m_cfg.randomNumbers) {
    throw std::invalid_argument("Missing random numbers tool");
  }
  if (m_cfg.digitizationConfigs.empty()) {
    throw std::invalid_argument("Missing digitization configuration");
  }

  if (m_cfg.doClusterization) {
    if (m_cfg.outputSourceLinks.empty()) {
      throw std::invalid_argument("Missing source links output collection");
    }
    if (m_cfg.outputMeasurements.empty()) {
      throw std::invalid_argument("Missing measurements output collection");
    }
    if (m_cfg.outputClusters.empty()) {
      throw std::invalid_argument("Missing cluster output collection");
    }
    if (m_cfg.outputMeasurementParticlesMap.empty()) {
      throw std::invalid_argument(
          "Missing hit-to-particles map output collection");
    }
    if (m_cfg.outputMeasurementSimHitsMap.empty()) {
      throw std::invalid_argument(
          "Missing hit-to-simulated-hits map output collection");
    }

    m_sourceLinkWriteHandle.initialize(m_cfg.outputSourceLinks);
    m_measurementWriteHandle.initialize(m_cfg.outputMeasurements);
    m_clusterWriteHandle.initialize(m_cfg.outputClusters);
    m_measurementParticlesMapWriteHandle.initialize(
        m_cfg.outputMeasurementParticlesMap);
    m_measurementSimHitsMapWriteHandle.initialize(
        m_cfg.outputMeasurementSimHitsMap);
  }

  if (m_cfg.doOutputCells) {
    if (m_cfg.outputCells.empty()) {
      throw std::invalid_argument("Missing cell output collection");
    }

    m_cellsWriteHandle.initialize(m_cfg.outputCells);
  }

  m_simContainerReadHandle.initialize(m_cfg.inputSimHits);

  // Create the digitizers from the configuration
  std::vector<std::pair<Acts::GeometryIdentifier, Digitizer>> digitizerInput;

  for (std::size_t i = 0; i < m_cfg.digitizationConfigs.size(); ++i) {
    GeometricConfig geoCfg;
    Acts::GeometryIdentifier geoId = m_cfg.digitizationConfigs.idAt(i);

    const auto& digiCfg = m_cfg.digitizationConfigs.valueAt(i);
    geoCfg = digiCfg.geometricDigiConfig;
    // Copy so we can sort in-place
    SmearingConfig smCfg = digiCfg.smearingDigiConfig;

    std::vector<Acts::BoundIndices> indices;
    for (auto& gcf : smCfg) {
      indices.push_back(gcf.index);
    }
    indices.insert(indices.begin(), geoCfg.indices.begin(),
                   geoCfg.indices.end());

    // Make sure the configured input parameter indices are sorted and unique
    std::ranges::sort(indices);

    auto dup = std::adjacent_find(indices.begin(), indices.end());
    if (dup != indices.end()) {
      std::invalid_argument(
          "Digitization configuration contains duplicate parameter indices");
    }

    switch (smCfg.size()) {
      case 0u:
        digitizerInput.emplace_back(geoId, makeDigitizer<0u>(digiCfg));
        break;
      case 1u:
        digitizerInput.emplace_back(geoId, makeDigitizer<1u>(digiCfg));
        break;
      case 2u:
        digitizerInput.emplace_back(geoId, makeDigitizer<2u>(digiCfg));
        break;
      case 3u:
        digitizerInput.emplace_back(geoId, makeDigitizer<3u>(digiCfg));
        break;
      case 4u:
        digitizerInput.emplace_back(geoId, makeDigitizer<4u>(digiCfg));
        break;
      default:
        throw std::invalid_argument("Unsupported smearer size");
    }
  }

  m_digitizers = Acts::GeometryHierarchyMap<Digitizer>(digitizerInput);
}

ActsExamples::ProcessCode ActsExamples::DigitizationAlgorithm::execute(
    const AlgorithmContext& ctx) const {
  // Retrieve input
  const auto& simHits = m_simContainerReadHandle(ctx);
  ACTS_DEBUG("Loaded " << simHits.size() << " sim hits");

  // Prepare output containers
  // need list here for stable addresses
  IndexSourceLinkContainer sourceLinks;
  MeasurementContainer measurements;
  ClusterContainer clusters;
  IndexMultimap<ActsFatras::Barcode> measurementParticlesMap;
  IndexMultimap<Index> measurementSimHitsMap;
  sourceLinks.reserve(simHits.size());
  measurements.reserve(simHits.size());
  measurementParticlesMap.reserve(simHits.size());
  measurementSimHitsMap.reserve(simHits.size());

  // Setup random number generator
  auto rng = m_cfg.randomNumbers->spawnGenerator(ctx);

  // Some statistics
  std::size_t skippedHits = 0;

  // Some algorithms do the clusterization themselves such as the traccc chain.
  // Thus we need to store the cell data from the simulation.
  CellsMap cellsMap;

  ACTS_DEBUG("Starting loop over modules ...");
  for (const auto& simHitsGroup : groupByModule(simHits)) {
    // Manual pair unpacking instead of using
    //   auto [moduleGeoId, moduleSimHits] : ...
    // otherwise clang on macos complains that it is unable to capture the local
    // binding in the lambda used for visiting the smearer below.
    Acts::GeometryIdentifier moduleGeoId = simHitsGroup.first;
    const auto& moduleSimHits = simHitsGroup.second;

    auto surfaceItr = m_cfg.surfaceByIdentifier.find(moduleGeoId);

    if (surfaceItr == m_cfg.surfaceByIdentifier.end()) {
      // this is either an invalid geometry id or a misconfigured smearer
      // setup; both cases can not be handled and should be fatal.
      ACTS_ERROR("Could not find surface " << moduleGeoId
                                           << " for configured smearer");
      return ProcessCode::ABORT;
    }

    const Acts::Surface* surfacePtr = surfaceItr->second;

    auto digitizerItr = m_digitizers.find(moduleGeoId);
    if (digitizerItr == m_digitizers.end()) {
      ACTS_VERBOSE("No digitizer present for module " << moduleGeoId);
      continue;
    } else {
      ACTS_VERBOSE("Digitizer found for module " << moduleGeoId);
    }

    // Run the digitizer. Iterate over the hits for this surface inside the
    // visitor so we do not need to lookup the variant object per-hit.
    std::visit(
        [&](const auto& digitizer) {
          ModuleClusters moduleClusters(
              digitizer.geometric.segmentation, digitizer.geometric.indices,
              m_cfg.doMerge, m_cfg.mergeNsigma, m_cfg.mergeCommonCorner);

          for (auto h = moduleSimHits.begin(); h != moduleSimHits.end(); ++h) {
            const auto& simHit = *h;
            const auto simHitIdx = simHits.index_of(h);

            DigitizedParameters dParameters;

            if (simHit.depositedEnergy() < m_cfg.minEnergyDeposit) {
              ACTS_VERBOSE("Skip hit because energy deposit to small");
              continue;
            }

            // Geometric part - 0, 1, 2 local parameters are possible
            if (!digitizer.geometric.indices.empty()) {
              ACTS_VERBOSE("Configured to geometric digitize "
                           << digitizer.geometric.indices.size()
                           << " parameters.");
              const auto& cfg = digitizer.geometric;
              Acts::Vector3 driftDir = cfg.drift(simHit.position(), rng);
              auto channelsRes = m_channelizer.channelize(
                  simHit, *surfacePtr, ctx.geoContext, driftDir,
                  cfg.segmentation, cfg.thickness);
              if (!channelsRes.ok() || channelsRes->empty()) {
                ACTS_DEBUG(
                    "Geometric channelization did not work, skipping this "
                    "hit.");
                continue;
              }
              ACTS_VERBOSE("Activated " << channelsRes->size()
                                        << " channels for this hit.");
              dParameters =
                  localParameters(digitizer.geometric, *channelsRes, rng);
            }

            // Smearing part - (optionally) rest
            if (!digitizer.smearing.indices.empty()) {
              ACTS_VERBOSE("Configured to smear "
                           << digitizer.smearing.indices.size()
                           << " parameters.");
              auto res =
                  digitizer.smearing(rng, simHit, *surfacePtr, ctx.geoContext);
              if (!res.ok()) {
                ++skippedHits;
                ACTS_DEBUG("Problem in hit smearing, skip hit ("
                           << res.error().message() << ")");
                continue;
              }
              const auto& [par, cov] = res.value();
              for (Eigen::Index ip = 0; ip < par.rows(); ++ip) {
                dParameters.indices.push_back(digitizer.smearing.indices[ip]);
                dParameters.values.push_back(par[ip]);
                dParameters.variances.push_back(cov(ip, ip));
              }
            }

            // Check on success - threshold could have eliminated all channels
            if (dParameters.values.empty()) {
              ACTS_VERBOSE(
                  "Parameter digitization did not yield a measurement.");
              continue;
            }

            moduleClusters.add(std::move(dParameters), simHitIdx);
          }

          auto digitizeParametersResult = moduleClusters.digitizedParameters();

          // Store the cell data into a map.
          if (m_cfg.doOutputCells) {
            std::vector<Cluster::Cell> cells;
            for (const auto& [dParameters, simhits] :
                 digitizeParametersResult) {
              for (const auto& cell : dParameters.cluster.channels) {
                cells.push_back(cell);
              }
            }
            cellsMap.insert({moduleGeoId, std::move(cells)});
          }

          if (m_cfg.doClusterization) {
            for (auto& [dParameters, simhits] : digitizeParametersResult) {
              // The measurement container is unordered and the index under
              // which the measurement will be stored is known before adding it.
              Index measurementIdx = measurements.size();
              IndexSourceLink sourceLink{moduleGeoId, measurementIdx};

              // Add to output containers:
              // index map and source link container are geometry-ordered.
              // since the input is also geometry-ordered, new items can
              // be added at the end.
              sourceLinks.insert(sourceLinks.end(), sourceLink);

              createMeasurement(measurements, dParameters, sourceLink);
              clusters.emplace_back(std::move(dParameters.cluster));
              // this digitization does hit merging so there can be more than
              // one mapping entry for each digitized hit.
              for (auto simHitIdx : simhits) {
                measurementParticlesMap.emplace_hint(
                    measurementParticlesMap.end(), measurementIdx,
                    simHits.nth(simHitIdx)->particleId());
                measurementSimHitsMap.emplace_hint(measurementSimHitsMap.end(),
                                                   measurementIdx, simHitIdx);
              }
            }
          }
        },
        *digitizerItr);
  }

  if (skippedHits > 0) {
    ACTS_WARNING(
        skippedHits
        << " skipped in Digitization. Enable DEBUG mode to see more details.");
  }

  if (m_cfg.doOutputCells) {
    m_cellsWriteHandle(ctx, std::move(cellsMap));
  }

  if (m_cfg.doClusterization) {
    m_sourceLinkWriteHandle(ctx, std::move(sourceLinks));
    m_measurementWriteHandle(ctx, std::move(measurements));
    m_clusterWriteHandle(ctx, std::move(clusters));
    m_measurementParticlesMapWriteHandle(ctx,
                                         std::move(measurementParticlesMap));
    m_measurementSimHitsMapWriteHandle(ctx, std::move(measurementSimHitsMap));
  }

  return ProcessCode::SUCCESS;
}

ActsExamples::DigitizedParameters
ActsExamples::DigitizationAlgorithm::localParameters(
    const GeometricConfig& geoCfg,
    const std::vector<ActsFatras::Segmentizer::ChannelSegment>& channels,
    RandomEngine& rng) const {
  DigitizedParameters dParameters;

  const auto& binningData = geoCfg.segmentation.binningData();

  Acts::ActsScalar totalWeight = 0.;
  Acts::Vector2 m(0., 0.);
  std::size_t b0min = std::numeric_limits<std::size_t>::max();
  std::size_t b0max = 0;
  std::size_t b1min = std::numeric_limits<std::size_t>::max();
  std::size_t b1max = 0;
  // Combine the channels
  for (const auto& ch : channels) {
    auto bin = ch.bin;
    Acts::ActsScalar charge =
        geoCfg.digital ? 1. : geoCfg.charge(ch.activation, rng);
    if (geoCfg.digital || charge > geoCfg.threshold) {
      totalWeight += charge;
      std::size_t b0 = bin[0];
      std::size_t b1 = bin[1];
      m += Acts::Vector2(charge * binningData[0].center(b0),
                         charge * binningData[1].center(b1));
      b0min = std::min(b0min, b0);
      b0max = std::max(b0max, b0);
      b1min = std::min(b1min, b1);
      b1max = std::max(b1max, b1);
      // Create a copy of the channel, as activation may change
      auto chdig = ch;
      chdig.bin = ch.bin;
      chdig.activation = charge;
      dParameters.cluster.channels.push_back(chdig);
    }
  }
  if (totalWeight > 0.) {
    m *= 1. / totalWeight;
    dParameters.indices = geoCfg.indices;
    for (auto idx : dParameters.indices) {
      dParameters.values.push_back(m[idx]);
    }
    std::size_t size0 = static_cast<std::size_t>(b0max - b0min + 1);
    std::size_t size1 = static_cast<std::size_t>(b1max - b1min + 1);

    dParameters.variances = geoCfg.variances({size0, size1}, {b0min, b1min});
    dParameters.cluster.sizeLoc0 = size0;
    dParameters.cluster.sizeLoc1 = size1;
  }

  return dParameters;
}
