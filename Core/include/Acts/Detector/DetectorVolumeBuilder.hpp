// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/ProtoBinning.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>

namespace Acts::Experimental {
class IExternalStructureBuilder;
class IInternalStructureBuilder;
class IGeometryIdGenerator;

/// @brief A generic detector volume builder that uses
/// an external builder for shape and portals and an internal
/// structure builder for volume internals.
///
/// @note Although this helper builds only a single volume,
/// it is to the outside presented as a DetectorComponent with
/// shell; like this it can be transparently be used for the
/// downstream detector construction process.
class DetectorVolumeBuilder : public IDetectorComponentBuilder {
 public:
  /// Nested configuration object
  struct Config {
    /// The name of the volume to be built
    std::string name = "unnamed";
    /// An external builder
    std::shared_ptr<const IExternalStructureBuilder> externalsBuilder = nullptr;
    /// An internal builder
    std::shared_ptr<const IInternalStructureBuilder> internalsBuilder = nullptr;
    /// The geometry id generator
    std::shared_ptr<const IGeometryIdGenerator> geoIdGenerator = nullptr;
    /// Material binning to be assigned to portals
    std::map<unsigned int, BinningDescription> portalMaterialBinning = {};
    /// Add eventual internal volume to root
    bool addInternalsToRoot = false;
    /// Auxiliary information
    std::string auxiliary = "";
  };

  /// Constructor with configuration arguments
  ///
  /// @param cfg is the configuration struct
  /// @param mlogger logging instance for screen output
  DetectorVolumeBuilder(const Config& cfg,
                        std::unique_ptr<const Logger> mlogger =
                            getDefaultLogger("DetectorVolumeBuilder",
                                             Logging::INFO));

  /// Final implementation of a volume builder that is purely defined
  /// by an internal and external structure builder
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
