// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Podio/PodioUtil.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsFatras/EventData/Hit.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <string>

namespace ActsExamples {

/// Write out the tracks reconstructed using Combinatorial Kalman Filter to
/// EDM4hep.
///
/// Inpersistent information:
/// - trajectory state incomplete
/// - relation to the particles
///
/// Known issues:
/// - curvature parameter
/// - track state local coordinates are written to (D0,Z0)
/// - covariance incorrect
class EDM4hepMultiTrajectoryWriter : public WriterT<TrajectoriesContainer> {
 public:
  struct Config {
    /// Input trajectory collection
    std::string inputTrajectories;
    /// Input hit-particles map collection
    std::string inputMeasurementParticlesMap;
    /// Where to place output file
    std::string outputPath;
    /// B field in the longitudinal direction
    double Bz{};
    /// Particle hypothesis
    Acts::ParticleHypothesis particleHypothesis =
        Acts::ParticleHypothesis::pion();
  };

  /// constructor
  /// @param config is the configuration object
  /// @param level is the output logging level
  EDM4hepMultiTrajectoryWriter(
      const Config& config, Acts::Logging::Level level = Acts::Logging::INFO);

  ProcessCode finalize() final;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] context is the algorithm context for consistency
  /// @param [in] tracks is the track collection
  ProcessCode writeT(const AlgorithmContext& context,
                     const TrajectoriesContainer& trajectories) final;

 private:
  Config m_cfg;

  std::mutex m_writeMutex;
  Acts::PodioUtil::ROOTWriter m_writer;

  ReadDataHandle<IndexMultimap<ActsFatras::Barcode>>
      m_inputMeasurementParticlesMap{this, "InputMeasurementParticlesMaps"};
};

}  // namespace ActsExamples
