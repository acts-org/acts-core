// This file is part of the Acts project.
//
// Copyright (C) 2022-2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Detector/interface/IInternalStructureBuilder.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>

namespace Acts {
namespace Experimental {

/// @brief A generic detector volume builder that uses
/// an external builder for shape and portals and an internal
/// structure builder for volume internals.
///
/// @note Although this vuilder builds only a single volume,
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
    std::shared_ptr<IExternalStructureBuilder> externalsBuilder = nullptr;
    /// An internal builder
    std::shared_ptr<IInternalStructureBuilder> internalsBuilder = nullptr;
    /// Add to the root volumes: the current volume
    bool addToRoot = true;
    /// Add eventual internal volume to root
    bool addInternalsToRoot = false;
    /// Auxilliary information
    std::string auxilliary = "";
  };

  /// Constructor with configuration argumetns
  ///
  /// @param cfg is the configuration struct
  /// @param logger logging instance for screen output
  DetectorVolumeBuilder(const Config& cfg,
                        std::unique_ptr<const Logger> logger = getDefaultLogger(
                            "DetectorVolumeBuilder", Logging::INFO));

  /// Final implementation of a volume builder that is purely defined
  /// by an internal and external structure builder
  ///
  /// @param roots [in,out] the detector root volumes
  /// @param gctx The geometry context for this call
  ///
  /// @return an outgoing detector component
  DetectorComponent construct(RootDetectorVolumes& roots,
                              const GeometryContext& gctx) const final;

 private:
  /// configuration object
  Config m_cfg;

  /// Private acces method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Experimental
}  // namespace Acts
