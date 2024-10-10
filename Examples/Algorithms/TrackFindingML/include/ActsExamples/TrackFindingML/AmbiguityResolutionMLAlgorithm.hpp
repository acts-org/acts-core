// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Plugins/Onnx/AmbiguityTrackClassifier.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/TrackFindingML/AmbiguityResolutionML.hpp"

#include <string>

namespace ActsExamples {

/// Evicts tracks that seem to be duplicated and fake.
///
/// The implementation works as follows:
///  1) Cluster together nearby tracks using shared hits
///  2) For each track use a neural network to compute a score
///  3) In each cluster keep the track with the highest score
class AmbiguityResolutionMLAlgorithm final : public AmbiguityResolutionML {
 public:
  struct Config {
    /// Input track collection.
    std::string inputTracks;
    /// Path to the ONNX model for the duplicate neural network
    std::string inputDuplicateNN;
    /// Output track collection.
    std::string outputTracks;
    /// Minimum number of measurement to form a track.
    int nMeasurementsMin = 7;
  };

  /// Construct the ambiguity resolution algorithm.
  ///
  /// @param cfg is the algorithm configuration
  /// @param lvl is the logging level
  AmbiguityResolutionMLAlgorithm(Config cfg, Acts::Logging::Level lvl);

  /// Run the ambiguity resolution algorithm.
  ///
  /// @param cxt is the algorithm context with event information
  /// @return a process code indication success or failure
  ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Const access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  // ONNX model for track selection
  Acts::AmbiguityTrackClassifier m_duplicateClassifier;
  ReadDataHandle<ConstTrackContainer> m_inputTracks{this, "InputTracks"};
  WriteDataHandle<ConstTrackContainer> m_outputTracks{this, "OutputTracks"};
};

}  // namespace ActsExamples
