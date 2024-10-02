// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Acts::Experimental {

class IRootVolumeFinderBuilder;
class IGeometryIdGenerator;

/// @brief A dedicated container builder for cuboid detector containers
///
/// It relies on the detailed implementation of the CuboidDetectorHelper
/// and allows for DetectorVolume attachment in x/y/z
///
/// There exists an option to create this container builder (recursively)
/// from a blueprint tree, which attempts to fill in the gap volumes
/// accordingly.
///
/// @note the builder expects a fully consistent set of sub volume builders
/// that will be executed in a chain
///
/// @note allowed BinningValue(s) for the cuboid container builder are
/// {binX}, {binY}, {binZ}.
///
/// @note Connecting containers isn't functional yet due to the underlying
/// issues in the CuboidDetectorHelper
///
class CuboidalContainerBuilder : public IDetectorComponentBuilder {
 public:
  /// Nested configuration object
  struct Config {
    /// The configured volume builders
    std::vector<std::shared_ptr<const IDetectorComponentBuilder>> builders = {};
    /// Binning prescription of attachment
    BinningValue binning{};
    /// The root volume finder
    std::shared_ptr<const IRootVolumeFinderBuilder> rootVolumeFinderBuilder =
        nullptr;
    /// The geometry id generator
    std::shared_ptr<const IGeometryIdGenerator> geoIdGenerator = nullptr;
    /// An eventual reverse geometry id generation
    bool geoIdReverseGen = false;
    /// Auxiliary information, mainly for screen output
    std::string auxiliary = "";
  };

  /// Constructor with configuration struct
  ///
  /// @param cfg is the configuration struct
  /// @param logger logging instance for screen output
  CuboidalContainerBuilder(const Config& cfg,
                           std::unique_ptr<const Logger> logger =
                               getDefaultLogger("CuboidalContainerBuilder",
                                                Logging::INFO));

  /// Constructor from blueprint and logging level
  ///
  /// It will create recursively the builders of sub volumes
  ///
  /// @param bpNode is the entry blue print node
  /// @param logLevel is the logging output level for the builder tools
  ///
  /// @note no checking is being done on consistency of the blueprint,
  /// it is assumed it has passed first through gap filling via the
  /// blueprint helper.
  ///
  /// @note that the naming of the builders is taken from the bluprint nodes
  ///
  /// @return a cylindrical container builder representing this blueprint
  CuboidalContainerBuilder(const Acts::Experimental::Blueprint::Node& bpNode,
                           Acts::Logging::Level logLevel = Acts::Logging::INFO);

  /// The final implementation of the cylindrical container builder
  ///
  /// @param gctx The geometry context for this call
  ///
  /// @return an outgoing detector component
  DetectorComponent construct(const GeometryContext& gctx) const final;

 private:
  /// configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts::Experimental
