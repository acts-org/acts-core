// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/IReader.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace Acts {
class Surface;
}

namespace ActsExamples {
struct AlgorithmContext;

/// Read in a measurement cluster collection in comma-separated-value format.
///
/// This reads three files per event file in the configured input
/// directory. By default it reads file in the current working directory.
/// Files are assumed to be named using the following schema
///
///     event000000001-cells.csv (optional)
///     event000000001-measurements.csv
///     event000000001-measurement-simhit-map.csv
///     event000000002-cells.csv (optional)
///     event000000002-measurements.csv
///     event000000001-measurement-simhit-map.csv
///
/// and each line in the file corresponds to one hit/cluster.
///
/// One file per fevent: thread-safe for parallel event processing.
class CsvMeasurementReader final : public IReader {
 public:
  struct Config {
    /// Where to read input files from.
    std::string inputDir;
    /// Output measurement collection.
    std::string outputMeasurements;
    /// Output measurement to sim hit collection.
    std::string outputMeasurementSimHitsMap;
    /// Output cluster collection (optional).
    std::string outputClusters;

    /// Input SimHits for measurement-particle map (optional)
    std::string inputSimHits;
    /// Output  measurement to particle collection (optional)
    /// @note Only filled if inputSimHits is given
    std::string outputMeasurementParticlesMap;
  };

  /// Construct the cluster reader.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  CsvMeasurementReader(const Config& config, Acts::Logging::Level level);

  std::string name() const override;

  /// Return the available events range.
  std::pair<std::size_t, std::size_t> availableEvents() const override;

  /// Read out data from the input stream.
  ProcessCode read(const ActsExamples::AlgorithmContext& ctx) override;

  /// Readonly access to the config
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  std::pair<std::size_t, std::size_t> m_eventsRange;
  std::unique_ptr<const Acts::Logger> m_logger;

  const Acts::Logger& logger() const { return *m_logger; }

  WriteDataHandle<MeasurementContainer> m_outputMeasurements{
      this, "OutputMeasurements"};

  WriteDataHandle<IndexMultimap<Index>> m_outputMeasurementSimHitsMap{
      this, "OutputMeasurementSimHitsMap"};

  WriteDataHandle<ClusterContainer> m_outputClusters{this, "OutputClusters"};

  WriteDataHandle<IndexMultimap<ActsFatras::Barcode>>
      m_outputMeasurementParticlesMap{this, "OutputMeasurementParticlesMap"};

  ReadDataHandle<SimHitContainer> m_inputHits{this, "InputHits"};
};

}  // namespace ActsExamples
