// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/TrackFinding/TrackParamsLookupAccumulator.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/TrackFinding/ITrackParamsLookupWriter.hpp"

#include <memory>

namespace ActsExamples {

/// @brief Algorithm to estimate track parameters lookup tables
///
/// This algorithm is used to estimate track parameters lookup tables
/// for track parameter estimation in seeding. The algorithm imposes
/// grids onto the reference tracking layers and accumulates track
/// parameters in the grid bins. The track parameters are then averaged
/// to create a lookup table for track parameter estimation in seeding.
class TrackParamsLookupEstimation : public IAlgorithm {
 public:
  using TrackParamsLookupAccumulator =
      Acts::TrackParamsLookupAccumulator<TrackParamsLookupGrid>;

  /// @brief Nested configuration struct
  struct Config {
    /// Reference tracking layers
    std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>
        refLayers;
    /// Binning of the grid to be emposed
    /// onto the reference layers
    std::pair<std::size_t, std::size_t> bins;
    /// Input SimHit container
    std::string inputHits = "InputHits";
    /// Input SimParticle container
    std::string inputParticles = "InputParticles";
    /// Track lookup writers
    std::vector<std::shared_ptr<ITrackParamsLookupWriter>>
        trackLookupGridWriters{};
  };

  /// @brief Constructor
  TrackParamsLookupEstimation(const Config& config, Acts::Logging::Level level);

  /// @brief The execute method
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  /// Configuration
  Config m_cfg;

  /// Input data handles
  ReadDataHandle<SimParticleContainer> m_inputParticles{this,
                                                        "InputSimParticles"};

  ReadDataHandle<SimHitContainer> m_inputSimHits{this, "InputSimHits"};

  /// Accumulators for the track parameters
  std::unordered_map<Acts::GeometryIdentifier,
                     std::unique_ptr<TrackParamsLookupAccumulator>>
      m_accumulators;
};

}  // namespace ActsExamples
