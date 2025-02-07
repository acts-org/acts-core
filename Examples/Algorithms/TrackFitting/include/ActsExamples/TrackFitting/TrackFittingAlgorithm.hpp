// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IAlgorithm.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/TrackFitting/TrackFitterFunction.hpp"

#include <memory>
#include <string>

namespace Acts {
class TrackingGeometry;
}

namespace ActsExamples {
class MeasurementCalibrator;
class TrackFitterFunction;
struct AlgorithmContext;

class TrackFittingAlgorithm final : public IAlgorithm {
 public:
  struct Config {
    /// Input measurements collection.
    std::string inputMeasurements;
    /// Input proto tracks collection, i.e. groups of hit indices.
    std::string inputProtoTracks;
    /// Input initial track parameter estimates for for each proto track.
    std::string inputInitialTrackParameters;
    /// (optional) Input clusters for each measurement
    std::string inputClusters;
    /// Output fitted tracks collection.
    std::string outputTracks;
    /// Type erased fitter function.
    std::shared_ptr<TrackFitterFunction> fit;
    /// Pick a single track for debugging (-1 process all tracks)
    int pickTrack = -1;
    // Type erased calibrator for the measurements
    std::shared_ptr<MeasurementCalibrator> calibrator;
  };

  /// Constructor of the fitting algorithm
  ///
  /// @param config is the config struct to configure the algorithm
  /// @param level is the logging level
  TrackFittingAlgorithm(Config config, Acts::Logging::Level level);

  /// Framework execute method of the fitting algorithm
  ///
  /// @param ctx is the algorithm context that holds event-wise information
  /// @return a process code to steer the algporithm flow
  ActsExamples::ProcessCode execute(const AlgorithmContext& ctx) const final;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;

  ReadDataHandle<MeasurementContainer> m_inputMeasurements{this,
                                                           "InputMeasurements"};
  ReadDataHandle<ProtoTrackContainer> m_inputProtoTracks{this,
                                                         "InputProtoTracks"};
  ReadDataHandle<TrackParametersContainer> m_inputInitialTrackParameters{
      this, "InputInitialTrackParameters"};

  ReadDataHandle<ClusterContainer> m_inputClusters{this, "InputClusters"};

  WriteDataHandle<ConstTrackContainer> m_outputTracks{this, "OutputTracks"};
};

}  // namespace ActsExamples
