// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "SeedingPerformanceWriter.hpp"

#include "Acts/Utilities/MultiIndex.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Utilities/EventDataTransforms.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"
#include "ActsFatras/EventData/Barcode.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <TFile.h>
#include <TVectorT.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::phi;

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

ActsExamples::SeedingPerformanceWriter::SeedingPerformanceWriter(
    ActsExamples::SeedingPerformanceWriter::Config config,
    Acts::Logging::Level level)
    : WriterT(config.inputSeeds, "SeedingPerformanceWriter", level),
      m_cfg(std::move(config)),
      m_effPlotTool(m_cfg.effPlotToolConfig, level),
      m_duplicationPlotTool(m_cfg.duplicationPlotToolConfig, level) {
  if (m_cfg.inputMeasurementParticlesMap.empty()) {
    throw std::invalid_argument("Missing hit-particles map input collection");
  }
  if (m_cfg.inputParticles.empty()) {
    throw std::invalid_argument("Missing input particles collection");
  }
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename");
  }

  m_inputParticles.initialize(m_cfg.inputParticles);
  m_inputMeasurementParticlesMap.initialize(m_cfg.inputMeasurementParticlesMap);

  // the output file can not be given externally since TFile accesses to the
  // same file from multiple threads are unsafe.
  // must always be opened internally
  auto path = m_cfg.filePath;
  m_outputFile = TFile::Open(path.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::invalid_argument("Could not open '" + path + "'");
  }
  // initialize the plot tools
  m_effPlotTool.book(m_effPlotCache);
  m_duplicationPlotTool.book(m_duplicationPlotCache);
}

ActsExamples::SeedingPerformanceWriter::~SeedingPerformanceWriter() {
  m_effPlotTool.clear(m_effPlotCache);
  m_duplicationPlotTool.clear(m_duplicationPlotCache);
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ActsExamples::ProcessCode ActsExamples::SeedingPerformanceWriter::finalize() {
  float eff = static_cast<float>(m_nTotalMatchedParticles) / m_nTotalParticles;
  float fakeRate =
      static_cast<float>(m_nTotalSeeds - m_nTotalMatchedSeeds) / m_nTotalSeeds;
  float duplicationRate = static_cast<float>(m_nTotalDuplicatedParticles) /
                          m_nTotalMatchedParticles;
  float aveNDuplicatedSeeds =
      static_cast<float>(m_nTotalMatchedSeeds - m_nTotalMatchedParticles) /
      m_nTotalMatchedParticles;
  float totalSeedPurity =
      static_cast<float>(m_nTotalMatchedSeeds) / m_nTotalSeeds;
  ACTS_DEBUG("nTotalSeeds               = " << m_nTotalSeeds);
  ACTS_DEBUG("nTotalMatchedSeeds        = " << m_nTotalMatchedSeeds);
  ACTS_DEBUG("nTotalParticles           = " << m_nTotalParticles);
  ACTS_DEBUG("nTotalMatchedParticles    = " << m_nTotalMatchedParticles);
  ACTS_DEBUG("nTotalDuplicatedParticles = " << m_nTotalDuplicatedParticles);

  ACTS_INFO("Efficiency (nMatchedParticles / nAllParticles) = " << eff);
  ACTS_INFO("Fake rate (nUnMatchedSeeds / nAllSeeds) = " << fakeRate);
  ACTS_INFO("Total seed purity (nTotalMatchedSeeds / m_nTotalSeeds)	= "
            << totalSeedPurity);
  ACTS_INFO(
      "Duplication rate (nDuplicatedMatchedParticles / nMatchedParticles) = "
      << duplicationRate);
  ACTS_INFO(
      "Average number of duplicated seeds ((nMatchedSeeds - nMatchedParticles) "
      "/ nMatchedParticles) = "
      << aveNDuplicatedSeeds);

  auto writeFloat = [&](float f, const char* name) {
    TVectorF v(1);
    v[0] = f;
    m_outputFile->WriteObject(&v, name);
  };

  if (m_outputFile != nullptr) {
    m_outputFile->cd();
    m_effPlotTool.write(m_effPlotCache);
    m_duplicationPlotTool.write(m_duplicationPlotCache);
    writeFloat(eff, "eff_seeds");
    writeFloat(fakeRate, "fakerate_seeds");
    writeFloat(duplicationRate, "duplicaterate_seeds");
    writeFloat(totalSeedPurity, "purity_seeds");
    ACTS_INFO("Wrote performance plots to '" << m_outputFile->GetPath() << "'");
  }
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::SeedingPerformanceWriter::writeT(
    const AlgorithmContext& ctx, const SimSeedContainer& seeds) {
  // Read truth information collections
  const auto& particles = m_inputParticles(ctx);
  const auto& hitParticlesMap = m_inputMeasurementParticlesMap(ctx);

  std::size_t nSeeds = seeds.size();
  std::size_t nMatchedSeeds = 0;
  // Map from particles to how many times they were successfully found by a seed
  std::unordered_map<ActsFatras::Barcode, std::size_t> truthCount;

  for (std::size_t itrack = 0; itrack < seeds.size(); ++itrack) {
    const auto& seed = seeds[itrack];
    const auto track = seedToPrototrack(seed);
    std::vector<ParticleHitCount> particleHitCounts;
    identifyContributingParticles(hitParticlesMap, track, particleHitCounts);
    // All hits matched to the same particle
    if (particleHitCounts.size() == 1) {
      auto prt = particleHitCounts.at(0);
      auto it = truthCount.try_emplace(prt.particleId, 0u).first;
      it->second += 1;
      nMatchedSeeds++;
    }
  }

  int nMatchedParticles = 0;
  int nDuplicatedParticles = 0;
  // Fill the efficiency and fake rate plots
  for (const auto& particle : particles) {
    const auto it1 = truthCount.find(particle.particleId());
    bool isMatched = false;
    int nMatchedSeedsForParticle = 0;
    if (it1 != truthCount.end()) {
      isMatched = true;
      nMatchedParticles++;
      nMatchedSeedsForParticle = it1->second;
      if (nMatchedSeedsForParticle > 1) {
        nDuplicatedParticles++;
      }
    }
    // Loop over all the other truth particle and find the distance to the
    // closest one
    double minDeltaR = -1;
    for (const auto& closeParticle : particles) {
      if (closeParticle.particleId() == particle.particleId()) {
        continue;
      }
      double p_phi = phi(particle.direction());
      double p_eta = eta(particle.direction());
      double c_phi = phi(closeParticle.direction());
      double c_eta = eta(closeParticle.direction());
      double distance = sqrt(pow(p_phi - c_phi, 2) + pow(p_eta - c_eta, 2));
      if (minDeltaR == -1 || distance < minDeltaR) {
        minDeltaR = distance;
      }
    }
    m_effPlotTool.fill(m_effPlotCache, particle, minDeltaR, isMatched);
    m_duplicationPlotTool.fill(m_duplicationPlotCache, particle,
                               nMatchedSeedsForParticle - 1);
  }
  ACTS_DEBUG("Number of seeds: " << nSeeds);
  m_nTotalSeeds += nSeeds;
  m_nTotalMatchedSeeds += nMatchedSeeds;
  m_nTotalParticles += particles.size();
  m_nTotalMatchedParticles += nMatchedParticles;
  m_nTotalDuplicatedParticles += nDuplicatedParticles;

  return ProcessCode::SUCCESS;
}
