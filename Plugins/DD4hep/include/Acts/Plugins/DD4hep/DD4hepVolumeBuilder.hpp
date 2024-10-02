// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/IConfinedTrackingVolumeBuilder.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <vector>

#include "DD4hep/DetElement.h"

class TrackingVolume;
namespace Acts {
class Logger;
}  // namespace Acts

using MutableTrackingVolumePtr = std::shared_ptr<TrackingVolume>;
using MutableTrackingVolumeVector = std::vector<MutableTrackingVolumePtr>;

class TGeoMatrix;

namespace Acts {

/// @brief build confined TrackingVolumes of one cylinder setup from DD4hep
/// input.
///
/// This class is an implementation of the Acts::IConfinedTrackingVolumeBuilder,
/// creating the central (volumes of barrel), the negative and positive volumes
/// (volumes of endcaps) of one hierarchy (e.g. ECal, HCal...) with input from
/// DD4hep.

class DD4hepVolumeBuilder : public IConfinedTrackingVolumeBuilder {
 public:
  /// @struct Config
  /// Nested configuration struct for steering of the volume builder
  struct Config {
    /// string based identification
    std::string configurationName = "undefined";
    /// Vector of central confined volumes
    std::vector<dd4hep::DetElement> centralVolumes;
  };

  /// Constructor
  /// @param [in] config is the configuration struct
  /// @param [in] logger is the logging instance
  DD4hepVolumeBuilder(const Acts::DD4hepVolumeBuilder::Config& config,
                      std::unique_ptr<const Logger> logger);

  /// Destructor
  ~DD4hepVolumeBuilder() override;

  /// @brief Builder method for cylindrical, confined volume
  ///
  /// @return The vector of TrackingVolumes at the central sector
  MutableTrackingVolumeVector centralVolumes() const final;

  /// Name identification
  /// @return The string based identification of this configuration
  const std::string& identification() const final;

  /// Set the configuration object
  /// @param [in] config is the configuration struct
  void setConfiguration(const Config& config);

  /// Get the configuration object
  /// @return The used configuration struct
  Config getConfiguration() const;

  /// Set logging instance
  /// @param [in] logger Logger in use
  void setLogger(std::unique_ptr<const Logger> logger);

 private:
  /// Configuration object
  Config m_cfg;

  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to the logger
  /// @return Used logger
  const Logger& logger() const { return *m_logger; }

  /// @brief Converter of the transformation of a volume from DD4hep to Acts
  /// formalism
  ///
  /// @param [in] tGeoTrans Transformation of the DD4hep DetElement
  /// @return Pointer to the corresponding Acts transformation
  Acts::Transform3 convertTransform(const TGeoMatrix* tGeoTrans) const;
};

inline const std::string& DD4hepVolumeBuilder::identification() const {
  return m_cfg.configurationName;
}

inline DD4hepVolumeBuilder::Config DD4hepVolumeBuilder::getConfiguration()
    const {
  return m_cfg;
}

}  // namespace Acts
