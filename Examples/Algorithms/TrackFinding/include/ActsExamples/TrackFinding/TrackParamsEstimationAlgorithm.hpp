// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSeed.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/MagneticField/MagneticField.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Acts {
class TrackingGeometry;
class MagneticFieldProvider;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

/// Estimate track parameters for track seeds.
///
/// The algorithm takes the either directly the seeds or indirectly the proto
/// tracks and space points, and source links container as input. The proto
/// track is basically a seed and its space points info could be retrieved from
/// the space point container. The source links container is necessary to
/// retrieve the geometry identifier of the module at which a space point is
/// located. It creates two additional container to the event store, i.e. the
/// estimated track parameters container and the proto tracks container storing
/// only those proto tracks with track parameters estimated.
class TrackParamsEstimationAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// Input seeds collection.
    std::string inputSeeds;
    /// Input prototracks (optional)
    std::string inputProtoTracks;
    /// Output estimated track parameters collection.
    std::string outputTrackParameters;
    /// Output seed collection - only seeds with successful parameter estimation
    /// are propagated (optional)
    std::string outputSeeds;
    /// Output prototrack collection - only tracks with successful parameter
    /// estimation are propagated (optional)
    std::string outputProtoTracks;

    /// Tracking geometry for surface lookup.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
    /// Magnetic field variant.
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField;

    /// The minimum magnetic field to trigger the track parameters estimation
    double bFieldMin = 0.1 * Acts::UnitConstants::T;

    /// Initial sigmas for the track parameters.
    std::array<double, 6> initialSigmas = {
        1 * Acts::UnitConstants::mm,
        1 * Acts::UnitConstants::mm,
        1 * Acts::UnitConstants::degree,
        1 * Acts::UnitConstants::degree,
        0 * Acts::UnitConstants::e / Acts::UnitConstants::GeV,
        1 * Acts::UnitConstants::ns};
    /// Relative pt resolution used for the initial sigma of q/p.
    double initialSigmaPtRel = 0.1;
    /// Inflate initial covariance.
    std::array<double, 6> initialVarInflation = {1., 1., 1., 1., 1., 1.};
    /// Inflate time covariance if no time measurement is available.
    double noTimeVarInflation = 100.;

    /// Particle hypothesis.
    Acts::ParticleHypothesis particleHypothesis =
        Acts::ParticleHypothesis::pion();
  };

  /// Construct the track parameters making algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  TrackParamsEstimationAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the track parameters making algorithm.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const override;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<SimSeedContainer> m_inputSeeds{this, "InputSeeds"};
  ReadDataHandle<ProtoTrackContainer> m_inputTracks{this, "InputTracks"};

  WriteDataHandle<TrackParametersContainer> m_outputTrackParameters{
      this, "OutputTrackParameters"};
  WriteDataHandle<SimSeedContainer> m_outputSeeds{this, "OutputSeeds"};
  WriteDataHandle<ProtoTrackContainer> m_outputTracks{this, "OutputTracks"};
};

}  // namespace ActsExamples
