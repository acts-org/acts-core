// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/RandomNumbers.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace ActsExamples {
class RandomNumbers;
struct AlgorithmContext;

/// @brief Smear track parameters.
///
/// Track parameters are smeared in the local frame. The `loc0` and `loc1`
/// parameters are always defined within that local frame and not globally. The
/// generated bound parameters are stored in the same order as the input
/// parameters.
class TrackParameterSmearing final : public IAlgorithm {
 public:
  struct Config {
    /// Input track parameters collection.
    std::string inputTrackParameters;
    /// Output smeared track parameters collection.
    std::string outputTrackParameters;

    /// Random numbers service.
    std::shared_ptr<const RandomNumbers> randomNumbers = nullptr;

    // Smearing parameters
    /// Constant term of the loc0 resolution.
    double sigmaLoc0 = 20 * Acts::UnitConstants::um;
    /// Pt-dependent loc0 resolution of the form sigma_loc0 =
    /// A*exp(-1.*abs(B)*pt).
    double sigmaLoc0PtA = 30 * Acts::UnitConstants::um;
    double sigmaLoc0PtB = 0.3 / Acts::UnitConstants::GeV;
    /// Constant term of the loc1 resolution.
    double sigmaLoc1 = 20 * Acts::UnitConstants::um;
    /// Pt-dependent loc1 resolution of the form sigma_loc1 =
    /// A*exp(-1.*abs(B)*pt).
    double sigmaLoc1PtA = 30 * Acts::UnitConstants::um;
    double sigmaLoc1PtB = 0.3 / Acts::UnitConstants::GeV;
    /// Time resolution.
    double sigmaTime = 1 * Acts::UnitConstants::ns;
    /// Phi angular resolution.
    double sigmaPhi = 1 * Acts::UnitConstants::degree;
    /// Theta angular resolution.
    double sigmaTheta = 1 * Acts::UnitConstants::degree;
    /// Relative transverse momentum resolution.
    double sigmaPtRel = 0.05;

    /// Optional. Initial sigmas for the track parameters which overwrites the
    /// smearing params if set.
    std::optional<std::array<double, 6>> initialSigmas;
    /// Relative pt resolution used for the initial sigma of q/p.
    double initialSigmaPtRel = 0.1;
    /// Inflate the initial covariance matrix
    std::array<double, 6> initialVarInflation = {1e4, 1e4, 1e4, 1e4, 1e4, 1e4};

    /// Optional particle hypothesis override.
    std::optional<Acts::ParticleHypothesis> particleHypothesis = std::nullopt;
  };

  TrackParameterSmearing(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<TrackParametersContainer> m_inputTrackParameters{
      this, "InputTrackParameters"};

  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};
};

}  // namespace ActsExamples
