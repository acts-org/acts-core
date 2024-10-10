// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/IWriter.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include <Acts/Geometry/TrackingGeometry.hpp>
#include <Acts/Utilities/Logger.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <string>

namespace Acts {
class TrackingVolume;
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {
struct AlgorithmContext;

/// Write out the geometry for all sensitive detector surfaces.
///
/// This writes a `detectors.csv` file at the end of the run using the
/// default context to determine the geometry. If configured, it also writes
/// an additional file for each event using the following schema
///
///     event000000001-detectors.csv
///     event000000002-detectors.csv
///     ...
///
/// that uses the per-event context to determine the geometry.
class CsvTrackingGeometryWriter : public IWriter {
 public:
  struct Config {
    /// The tracking geometry that should be written.
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry;
    /// Where to place output files.
    std::string outputDir;
    /// Number of decimal digits for floating point precision in output.
    std::size_t outputPrecision = std::numeric_limits<float>::max_digits10;
    /// Write sensitive surfaces
    bool writeSensitive = true;
    /// Write boundary surfaces
    bool writeBoundary = false;
    /// Write the surface grid information
    bool writeSurfaceGrid = true;
    /// Write the layer volume representation
    bool writeLayerVolume = true;
    /// Whether to write the per-event file.
    bool writePerEvent = false;
  };

  /// Construct the geometry writer.
  ///
  /// @param config is the configuration object
  /// @param level is the logging level
  CsvTrackingGeometryWriter(const Config& config, Acts::Logging::Level level);

  std::string name() const override;

  /// Write geometry using the per-event context (optional).
  ProcessCode write(const AlgorithmContext& ctx) override;

  /// Write geometry using the default context.
  ProcessCode finalize() override;

  /// Get readonly access to the config parameters
  const Config& config() const { return m_cfg; }

 private:
  Config m_cfg;
  const Acts::TrackingVolume* m_world = nullptr;
  std::unique_ptr<const Acts::Logger> m_logger;

  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
