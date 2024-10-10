// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Vertexing/FullBilloirVertexFitter.hpp"
#include "Acts/Vertexing/HelicalTrackLinearizer.hpp"
#include "Acts/Vertexing/Vertex.hpp"
#include "Acts/Vertexing/VertexingOptions.hpp"
#include "ActsExamples/EventData/ProtoVertex.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsExamples/EventData/Vertex.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Acts {
class MagneticFieldProvider;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

class VertexFitterAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// Optional. Input track parameters collection
    std::string inputTrackParameters;
    /// Input proto vertex collection
    std::string inputProtoVertices;
    /// Output vertex collection
    std::string outputVertices;
    /// The magnetic field
    std::shared_ptr<Acts::MagneticFieldProvider> bField;
    /// Constraint vertex fit bool
    bool doConstrainedFit = false;
    /// Vertex constraint position
    Acts::Vector4 constraintPos = Acts::Vector4(0, 0, 0, 0);
    /// Vertex constraint covariance matrix
    Acts::SquareMatrix4 constraintCov =
        Acts::Vector4(1e2 * Acts::UnitConstants::mm * Acts::UnitConstants::mm,
                      1e2 * Acts::UnitConstants::mm * Acts::UnitConstants::mm,
                      1e2 * Acts::UnitConstants::mm * Acts::UnitConstants::mm,
                      1e8 * Acts::UnitConstants::mm * Acts::UnitConstants::mm)
            .asDiagonal();
  };

  VertexFitterAlgorithm(const Config& cfg, Acts::Logging::Level lvl);

  /// Fit the input vertices.
  ///
  /// @param ctx is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<TrackParametersContainer> m_inputTrackParameters{
      this, "InputTrackParameters"};

  ReadDataHandle<ProtoVertexContainer> m_inputProtoVertices{
      this, "InputProtoVertices"};

  WriteDataHandle<VertexContainer> m_outputVertices{this, "OutputVertices"};
};

}  // namespace ActsExamples
