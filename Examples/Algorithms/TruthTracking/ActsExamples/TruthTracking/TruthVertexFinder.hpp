// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/ProtoVertex.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <string>

namespace ActsExamples {
struct AlgorithmContext;

/// Group particles into proto vertices using truth information.
class TruthVertexFinder final : public IAlgorithm {
 public:
  using HitParticlesMap = ActsExamples::IndexMultimap<ActsFatras::Barcode>;

  struct Config {
    /// The input tracks that should be used to create proto vertices.
    std::string inputTracks;
    /// The input truth particles that should be used to create proto vertices.
    std::string inputParticles;
    /// Input hit-particles map collection.
    std::string inputMeasurementParticlesMap;
    /// The output proto vertices collection.
    std::string outputProtoVertices;
    /// Exclude secondary particles not originating from the primary vertex.
    bool excludeSecondaries = true;
    /// Build separate proto vertices for the secondary particles.
    bool separateSecondaries = false;
    /// The minimum number of tracks required to create a proto vertex.
    double trackMatchingRatio = 0.5;
  };

  TruthVertexFinder(const Config& config, Acts::Logging::Level level);

  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<ConstTrackContainer> m_inputTracks{this, "InputTracks"};
  ReadDataHandle<SimParticleContainer> m_inputParticles{this, "InputParticles"};
  ReadDataHandle<HitParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};
  WriteDataHandle<ProtoVertexContainer> m_outputProtoVertices{
      this, "OutputProtoVertices"};
};

}  // namespace ActsExamples
