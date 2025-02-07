// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"

#include <limits>
#include <string>

namespace ActsExamples {

/// Select hits by applying some selection cuts.
class HitSelector final : public IAlgorithm {
 public:
  struct Config {
    /// Input hit collection.
    std::string inputHits;
    /// Optional input particle collection.
    std::string inputParticlesSelected;
    /// Output hit collection
    std::string outputHits;

    /// Min x cut
    double minX = -std::numeric_limits<double>::max();
    /// Max x cut
    double maxX = std::numeric_limits<double>::max();

    /// Min y cut
    double minY = -std::numeric_limits<double>::max();
    /// Max y cut
    double maxY = std::numeric_limits<double>::max();

    /// Min z cut
    double minZ = -std::numeric_limits<double>::max();
    /// Max z cut
    double maxZ = std::numeric_limits<double>::max();

    /// Min r cut
    double minR = 0.0;
    /// Max r cut
    double maxR = std::numeric_limits<double>::max();

    /// Min time cut
    double minTime = -std::numeric_limits<double>::max();
    /// Max time cut
    double maxTime = std::numeric_limits<double>::max();

    /// Min energy loss cut
    double minEnergyLoss = 0;
    /// Max energy loss cut
    double maxEnergyLoss = std::numeric_limits<double>::max();

    /// Min primary vertex ID cut
    std::uint64_t minPrimaryVertexId = 0;
    /// Max primary vertex ID cut
    std::uint64_t maxPrimaryVertexId =
        std::numeric_limits<std::uint64_t>::max();
  };

  HitSelector(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimHitContainer> m_inputHits{this, "InputHits"};
  ReadDataHandle<SimParticleContainer> m_inputParticlesSelected{
      this, "InputParticlesSelected"};
  WriteDataHandle<SimHitContainer> m_outputHits{this, "OutputHits"};
};

}  // namespace ActsExamples
