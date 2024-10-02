// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/LayerStructureBuilder.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace Acts::Experimental {

class MultiWireStructureBuilder {
 public:
  /// @brief Configuration struct for the MultiWireStructure Builder

  struct Config {
    /// The name of the detector volume component
    std::string name = "";

    /// The surfaces of the Multi Wire
    std::vector<std::shared_ptr<Acts::Surface>> mlSurfaces = {};

    /// The transform of the Multi Wire
    Transform3 transform = Transform3::Identity();

    /// The bounds of the multi-wire volume
    std::vector<ActsScalar> mlBounds = {};

    // The binning of the multi wire structure
    std::vector<ProtoBinning> mlBinning = {};

    /// A tolerance config
    float toleranceOverlap = 10.;
  };

  /// Constructor
  /// @param config The configure of the MultiWireStructureBuilder
  /// @param logger logging instance for screen output

  MultiWireStructureBuilder(
      const Config& config,
      std::unique_ptr<const Acts::Logger> logger = Acts::getDefaultLogger(
          "MultiWireStructureBuilder", Acts::Logging::VERBOSE));

  ~MultiWireStructureBuilder() = default;

  /// Construct the detector component

  /// @param gctx The Geometry Context of the current geometry
  /// @return a detector component object with the detector volume of the multilayer

  Acts::Experimental::DetectorComponent construct(
      const Acts::GeometryContext& gctx);

 private:
  Config mCfg;

  const Acts::Logger& logger() const { return *mLogger; }

  std::unique_ptr<const Acts::Logger> mLogger;
};

}  // namespace Acts::Experimental
