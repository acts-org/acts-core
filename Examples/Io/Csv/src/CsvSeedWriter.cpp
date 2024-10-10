// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Csv/CsvSeedWriter.hpp"

#include "Acts/EventData/Seed.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "ActsExamples/EventData/AverageSimHits.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/Utilities/EventDataTransforms.hpp"
#include "ActsExamples/Utilities/Paths.hpp"
#include "ActsExamples/Utilities/Range.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"

#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::phi;
using Acts::VectorHelpers::theta;

ActsExamples::CsvSeedWriter::CsvSeedWriter(
    const ActsExamples::CsvSeedWriter::Config& config,
    Acts::Logging::Level level)
    : WriterT<TrackParametersContainer>(config.inputTrackParameters,
                                        "CsvSeedWriter", level),
      m_cfg(config) {
  if (m_cfg.inputSimSeeds.empty()) {
    throw std::invalid_argument("Missing space points input collection");
  }
  if (m_cfg.inputSimHits.empty()) {
    throw std::invalid_argument("Missing simulated hits input collection");
  }
  if (m_cfg.inputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument("Missing hit-particles map input collection");
  }
  if (m_cfg.inputMeasurementSimHitsMap.empty()) {
    throw std::invalid_argument(
        "Missing hit-simulated-hits map input collection");
  }
  if (m_cfg.fileName.empty()) {
    throw std::invalid_argument("Missing output filename");
  }
  if (m_cfg.outputDir.empty()) {
    throw std::invalid_argument("Missing output directory");
  }

  m_inputSimSeeds.initialize(m_cfg.inputSimSeeds);
  m_inputSimHits.initialize(m_cfg.inputSimHits);
  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);
  m_inputMeasurementSimHitsMap.initialize(m_cfg.inputMeasurementSimHitsMap);
}

ActsExamples::ProcessCode ActsExamples::CsvSeedWriter::writeT(
    const ActsExamples::AlgorithmContext& ctx,
    const TrackParametersContainer& trackParams) {
  // Read additional input collections
  const auto& seeds = m_inputSimSeeds(ctx);
  const auto& simHits = m_inputSimHits(ctx);
  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(ctx);
  const auto& hitSimHitsMap = m_inputMeasurementSimHitsMap(ctx);

  std::string path =
      perEventFilepath(m_cfg.outputDir, m_cfg.fileName, ctx.eventNumber);

  std::ofstream mos(path, std::ofstream::out | std::ofstream::trunc);
  if (!mos) {
    throw std::ios_base::failure("Could not open '" + path + "' to write");
  }

  std::unordered_map<std::size_t, SeedInfo> infoMap;
  std::unordered_map<ActsFatras::Barcode, std::pair<std::size_t, float>>
      goodSeed;

  // Loop over the estimated track parameters
  for (std::size_t iparams = 0; iparams < trackParams.size(); ++iparams) {
    // The estimated bound parameters vector
    const auto params = trackParams[iparams].parameters();

    float seedPhi = params[Acts::eBoundPhi];
    float seedEta = std::atanh(std::cos(params[Acts::eBoundTheta]));

    // Get the proto track from which the track parameters are estimated
    const auto& seed = seeds[iparams];
    const auto& ptrack = seedToPrototrack(seed);

    std::vector<ParticleHitCount> particleHitCounts;
    identifyContributingParticles(hitParticlesMap, ptrack, particleHitCounts);
    bool truthMatched = false;
    float truthDistance = -1;
    auto majorityParticleId = particleHitCounts.front().particleId;
    // Seed are considered truth matched if they have only one contributing
    // particle
    if (particleHitCounts.size() == 1) {
      truthMatched = true;
      // Get the index of the first space point
      const auto& hitIdx = ptrack.front();
      // Get the sim hits via the measurement to sim hits map
      auto indices = makeRange(hitSimHitsMap.equal_range(hitIdx));
      // Get the truth particle direction from the sim hits
      Acts::Vector3 truthUnitDir = {0, 0, 0};
      for (auto [_, simHitIdx] : indices) {
        const auto& simHit = *simHits.nth(simHitIdx);
        if (simHit.particleId() == majorityParticleId) {
          truthUnitDir = simHit.direction();
        }
      }
      // Compute the distance between the truth and estimated directions
      float truthPhi = phi(truthUnitDir);
      float truthEta = std::atanh(std::cos(theta(truthUnitDir)));
      float dEta = fabs(truthEta - seedEta);
      float dPhi = fabs(truthPhi - seedPhi) < M_PI
                       ? fabs(truthPhi - seedPhi)
                       : fabs(truthPhi - seedPhi) - M_PI;
      truthDistance = sqrt(dPhi * dPhi + dEta * dEta);
      // If the seed is truth matched, check if it is the closest one for the
      // contributing particle
      if (goodSeed.contains(majorityParticleId)) {
        if (goodSeed[majorityParticleId].second > truthDistance) {
          goodSeed[majorityParticleId] = std::make_pair(iparams, truthDistance);
        }
      } else {
        goodSeed[majorityParticleId] = std::make_pair(iparams, truthDistance);
      }
    }
    // Store the global position of the space points
    boost::container::small_vector<Acts::Vector3, 3> globalPosition;
    for (auto spacePointPtr : seed.sp()) {
      Acts::Vector3 pos(spacePointPtr->x(), spacePointPtr->y(),
                        spacePointPtr->z());
      globalPosition.push_back(pos);
    }

    // track info
    SeedInfo toAdd;
    toAdd.seedID = iparams;
    toAdd.particleId = majorityParticleId;
    toAdd.seedPt = std::abs(1.0 / params[Acts::eBoundQOverP]) *
                   std::sin(params[Acts::eBoundTheta]);
    toAdd.seedPhi = seedPhi;
    toAdd.seedEta = seedEta;
    toAdd.vertexZ = seed.z();
    toAdd.quality = seed.seedQuality();
    toAdd.globalPosition = globalPosition;
    toAdd.truthDistance = truthDistance;
    toAdd.seedType = truthMatched ? "duplicate" : "fake";
    toAdd.measurementsID = ptrack;

    infoMap[toAdd.seedID] = toAdd;
  }

  mos << "seed_id,particleId," << "pT,eta,phi," << "bX,bY,bZ," << "mX,mY,mZ,"
      << "tX,tY,tZ," << "good/duplicate/fake," << "vertexZ,quality,"
      << "Hits_ID" << '\n';

  for (auto& [id, info] : infoMap) {
    if (goodSeed[info.particleId].first == id) {
      info.seedType = "good";
    }
    // write the track info
    mos << info.seedID << ",";
    mos << info.particleId << ",";
    mos << info.seedPt << ",";
    mos << info.seedEta << ",";
    mos << info.seedPhi << ",";
    for (auto& point : info.globalPosition) {
      mos << point.x() << ",";
      mos << point.y() << ",";
      mos << point.z() << ",";
    }
    mos << info.seedType << ",";
    mos << info.vertexZ << ",";
    mos << info.quality << ",";
    mos << "\"[";
    for (auto& ID : info.measurementsID) {
      mos << ID << ",";
    }
    mos << "]\"";
    mos << '\n';
  }

  return ProcessCode::SUCCESS;
}
