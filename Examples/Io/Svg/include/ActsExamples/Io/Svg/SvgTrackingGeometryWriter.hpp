// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/ProcessCode.hpp"
#include "ActsExamples/Io/Svg/SvgDefaults.hpp"
#include <Acts/Plugins/ActSVG/TrackingGeometrySvgConverter.hpp>
#include <Acts/Utilities/Logger.hpp>

#include <fstream>
#include <iostream>
#include <mutex>

namespace Acts {
class TrackingVolume;
class TrackingGeometry;
}  // namespace Acts

namespace ActsExamples {

/// @class SvgTrackingGeometryWriter
///
/// An Svg writer for the geometry: TrackingGeometry master
/// It delegates the writing to the converter from the Svg plugin
class SvgTrackingGeometryWriter {
 public:
  /// @class Config
  ///
  /// The nested config class for this writer
  class Config {
   public:
    Acts::Svg::TrackingGeometryConverter::Options converterOptions =
        s_trackingGeometryOptions;

    std::string outputDir = "";
  };

  /// Constructor
  /// @param config is the configuration class
  /// @param level the log level
  SvgTrackingGeometryWriter(const Config& config, Acts::Logging::Level level);

  /// Framework name() method
  /// @return the name of the tool
  std::string name() const;

  /// The write interface
  /// @param context the Algorithm/Event context of this call
  /// @param tGeometry is the geometry to be written out
  /// @return ProcessCode to indicate success/failure
  ActsExamples::ProcessCode write(const AlgorithmContext& context,
                                  const Acts::TrackingGeometry& tGeometry);

 private:
  std::unique_ptr<const Acts::Logger> m_logger;  ///< the logger instance

  Config m_cfg;  ///< the config class

  std::mutex m_writeMutex;  ///< Mutex used to protect multi-threaded writes

  /// Private access to the logging instance
  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
