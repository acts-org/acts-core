// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/ITrackingGeometryBuilder.hpp"
#include "Acts/Geometry/ITrackingVolumeHelper.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace Acts {

class TrackingVolume;
class TrackingGeometry;
class IMaterialDecorator;

/// The Acts::TrackingGeometry Builder for volumes that wrap around another
///
/// It retrieves an array of std::functions that build the TrackingGeometry
/// sequentially in order, with the following options:
/// - contained (e.g. a final insertion of a beam pipe of longer extend)
/// - wrapped (e.g. an outer detector wrapping an inner one)
/// - attached (e.g. a neighbor detector attaching to the previous one)
///
/// The returned volume of each step must be processable by the previous step
class TrackingGeometryBuilder : public ITrackingGeometryBuilder {
 public:
  /// @struct Config
  /// Nested Configuration for the CylinderVolumeBuilder
  struct Config {
    /// The list of tracking volume builders
    std::vector<std::function<std::shared_ptr<TrackingVolume>(
        const GeometryContext& gctx, const TrackingVolumePtr&,
        const std::shared_ptr<const VolumeBounds>&)>>
        trackingVolumeBuilders;

    /// The tracking volume helper for detector construction
    std::shared_ptr<const ITrackingVolumeHelper> trackingVolumeHelper = nullptr;

    /// The optional material decorator for this
    std::shared_ptr<const IMaterialDecorator> materialDecorator = nullptr;

    /// Optional geometry identifier hook to be used during closure
    std::shared_ptr<const GeometryIdentifierHook> geometryIdentifierHook =
        std::make_shared<GeometryIdentifierHook>();
  };

  /// Constructor
  ///
  /// @param [in] cgbConfig is the configuration struct for this builder
  /// @param [in] logger logging instance
  TrackingGeometryBuilder(const Config& cgbConfig,
                          std::unique_ptr<const Logger> logger =
                              getDefaultLogger("TrackingGeometryBuilder",
                                               Logging::INFO));

  /// Destructor
  ~TrackingGeometryBuilder() override = default;

  /// TrackingGeometry Interface method
  ///
  /// @param gctx geometry context of that building call
  ///
  /// @return a unique pointer to a TrackingGeometry
  std::unique_ptr<const TrackingGeometry> trackingGeometry(
      const GeometryContext& gctx) const final;

  /// Set configuration method
  ///
  /// @param cgbConfig is the new configuration struct
  void setConfiguration(const Config& cgbConfig);

  /// Get configuration method
  /// @return the current configuration
  const Config& getConfiguration() const;

  /// set logging instance
  /// @param newLogger the new logging instance
  void setLogger(std::unique_ptr<const Logger> newLogger);

 private:
  /// Configuration member
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// the logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts
