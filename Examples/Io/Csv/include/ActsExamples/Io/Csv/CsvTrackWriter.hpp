// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Framework/WriterT.hpp"
#include "ActsFatras/EventData/Barcode.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ActsExamples {

/// @class CsvTrackWriter
///
/// Write out the tracks reconstructed using Combinatorial Kalman Filter in
/// comma-separated-value format.
///
/// This writes one file per event into the configured output directory. By
/// default it writes to the current working directory.
/// Files are named using the following schema
///
///     event000000001-tracks_{algorithm_name}.csv
///     event000000002-tracks_{algorithm_name}.csv
///
/// and each line in the file corresponds to one track.
class CsvTrackWriter : public WriterT<ConstTrackContainer> {
 public:
  struct Config {
    /// Input track collection
    std::string inputTracks;
    /// where to place output files
    std::string outputDir;
    /// name of the output files
    std::string fileName = "CKFtracks.csv";
    /// Input hit-particles map collection
    std::string inputMeasurementParticlesMap;
    /// floating point precision
    std::size_t outputPrecision = 6;
    /// Min number of measurements
    std::size_t nMeasurementsMin = 7;
    /// Only write truth matched tracks
    bool onlyTruthMatched = false;
    /// Probability threshold for fake tracks
    double truthMatchProbMin = 0.5;
    /// Min pt of tracks
    double ptMin = 1 * Acts::UnitConstants::GeV;
  };

  /// constructor
  /// @param config is the configuration object
  /// @param level is the output logging level
  explicit CsvTrackWriter(const Config& config,
                          Acts::Logging::Level level = Acts::Logging::INFO);

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 protected:
  /// @brief Write method called by the base class
  /// @param [in] context is the algorithm context for consistency
  /// @param [in] tracks is the track collection
  ProcessCode writeT(const AlgorithmContext& context,
                     const ConstTrackContainer& tracks) override;

 private:
  /// Nested configuration struct
  Config m_cfg;

  ReadDataHandle<MeasurementParticlesMap> m_inputMeasurementParticlesMap{
      this, "InputMeasurementParticlesMap"};

  /// @brief Struct for brief trajectory summary info
  ///
  struct TrackInfo : public Acts::MultiTrajectoryHelpers::TrajectoryState {
    std::size_t trackId = 0;
    unsigned int seedID = 0;
    ActsFatras::Barcode particleId;
    std::size_t nMajorityHits = 0;
    std::string trackType;
    double truthMatchProb = 0;
    std::optional<TrackParameters> fittedParameters;
    std::vector<std::uint64_t> measurementsID;
  };
};

}  // namespace ActsExamples
