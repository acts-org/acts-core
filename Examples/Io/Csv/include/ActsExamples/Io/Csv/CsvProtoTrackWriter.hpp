// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/Digitization/DigitizationConfig.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/SimSpacePoint.hpp"
#include "ActsExamples/Framework/WriterT.hpp"

#include <string>

namespace ActsExamples {

/// @class CsvProtoTrackWriter
///
class CsvProtoTrackWriter final : public WriterT<ProtoTrackContainer> {
 public:
  struct Config {
    /// Which prototracks to write
    std::string inputPrototracks;
    /// Spacepoint collection
    std::string inputSpacepoints;
    /// Output directory
    std::string outputDir;
    /// Number of decimal digits for floating point precision in output.
    int outputPrecision = std::numeric_limits<float>::max_digits10;
  };

  /// Constructor with
  /// @param config configuration struct
  /// @param level logging level
  CsvProtoTrackWriter(const Config& config, Acts::Logging::Level level);

  /// Virtual destructor
  ~CsvProtoTrackWriter() override;

  /// End-of-run hook
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 protected:
  /// This implementation holds the actual writing method
  /// and is called by the WriterT<>::write interface
  ///
  /// @param ctx The Algorithm context with per event information
  /// @param measurements is the data to be written out
  ProcessCode writeT(const AlgorithmContext& ctx,
                     const ProtoTrackContainer& tracks) override;

 private:
  Config m_cfg;

  ReadDataHandle<SimSpacePointContainer> m_inputSpacepoints{this,
                                                            "inputSpacepoints"};
};

}  // namespace ActsExamples
