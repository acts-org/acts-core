// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/ITrackingVolumeArrayCreator.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <utility>

namespace Acts {

using TrackingVolumeOrderPosition = std::pair<TrackingVolumePtr, Vector3>;

///@class TrackingVolumeArrayCreator
///
/// The TrackingVolumeArrayCreator is a simple Tool that helps to construct
/// binned arrays of TrackingVolumes for both, confinement in another volume
/// and navigation issues.
///
class TrackingVolumeArrayCreator : public ITrackingVolumeArrayCreator {
 public:
  /// @brief This struct stores the configuration of the tracking geometry
  struct Config {};

  /// Constructor
  ///
  /// @param logger logging instance
  TrackingVolumeArrayCreator(const Config& /*cfg*/,
                             std::unique_ptr<const Logger> logger =
                                 getDefaultLogger("LayerArrayCreator",
                                                  Logging::INFO))
      : m_logger(std::move(logger)) {}

  /// create a tracking volume array
  ///
  /// @param [in] gctx the geometry context for this building
  /// @param [in] tVolumes is the vector of TrackingVolumes to be
  /// @param [in] bValue is the binning value
  ///
  /// @return new created volume array
  std::shared_ptr<const TrackingVolumeArray> trackingVolumeArray(
      const GeometryContext& gctx, const TrackingVolumeVector& tVolumes,
      BinningValue bValue) const override;

  /// Set logging instance
  ///
  /// @param logger is the logging instance to be set
  void setLogger(std::unique_ptr<const Logger> logger) {
    m_logger = std::move(logger);
  }

 private:
  // Private access to the logger method
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};
}  // namespace Acts
