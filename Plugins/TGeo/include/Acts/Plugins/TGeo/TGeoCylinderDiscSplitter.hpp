// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/TGeo/ITGeoDetectorElementSplitter.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <vector>

class TGeoNode;

namespace Acts {

class TGeoDetectorElement;

/// @brief TGeoCylinderDiscSplitter
///
/// Split Cylinder and disks into submodules
class TGeoCylinderDiscSplitter : public ITGeoDetectorElementSplitter {
 public:
  /// Nested configuration struct
  struct Config {
    /// Number of segments in phi for a disc
    int cylinderPhiSegments = -1;
    /// Number of segments in r for a disk
    int cylinderLongitudinalSegments = -1;

    /// Number of segments in phi for a disc
    int discPhiSegments = -1;
    /// Number of segments in r for a disk
    int discRadialSegments = -1;
  };

  /// Constructor
  ///
  /// @param cfg the configuration struct
  /// @param logger the logging object
  TGeoCylinderDiscSplitter(
      const Config& cfg,
      std::unique_ptr<const Acts::Logger> logger = Acts::getDefaultLogger(
          "TGeoCylinderDiscSplitter", Acts::Logging::INFO));

  ~TGeoCylinderDiscSplitter() override = default;

  /// Take a geometry context and TGeoElement and split it into sub elements
  ///
  /// @param gctx is a geometry context object
  /// @param tgde is a TGeoDetectorElement that is eventually split
  ///
  /// @note If no split is performed the unsplit detector element is returned
  ///
  /// @return a vector of TGeoDetectorElement objects
  std::vector<std::shared_ptr<const Acts::TGeoDetectorElement>> split(
      const GeometryContext& gctx,
      std::shared_ptr<const Acts::TGeoDetectorElement> tgde) const override;

 private:
  Config m_cfg;

  /// Private access to the logger
  const Acts::Logger& logger() const { return *m_logger; }

  /// Logging instance
  std::unique_ptr<const Acts::Logger> m_logger;
};

}  // namespace Acts
