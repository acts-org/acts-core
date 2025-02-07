// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/DetectorComponents.hpp"
#include "Acts/Detector/interface/IExternalStructureBuilder.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <optional>
#include <string>

namespace Acts::Experimental {

/// This class provides the external detector volume structure, configured
/// either from:
///
///  - a volume extent
///  - from an array with volume bounds identification
///
/// @note A starting transform (defaulted to identity) can be provided,
/// in case of volume bounds construction from an ```Acts::Extent```
/// object, the transform steming from the extent definition will
/// be applied on top of the provided starting transform.
/// In case of volume bounds construction from an array of values,
/// the starting transform is already the final volume placement.
class VolumeStructureBuilder : public IExternalStructureBuilder {
 public:
  /// Nexted configuration struct
  struct Config {
    /// A defined volume bound type
    VolumeBounds::BoundsType boundsType = VolumeBounds::BoundsType::eOther;
    /// The starting transform
    Transform3 transform = Transform3::Identity();
    /// The values (if already defined)
    std::vector<double> boundValues = {};
    /// The optional extent to feed into the values
    std::optional<Extent> extent = std::nullopt;
    /// Some auxiliary information
    std::string auxiliary = "";
  };

  /// Constructor
  ///
  /// @param cfg is the configuration struct
  /// @param mlogger logging instance for screen output
  VolumeStructureBuilder(const Config& cfg,
                         std::unique_ptr<const Logger> mlogger =
                             getDefaultLogger("VolumeStructureBuilder",
                                              Logging::INFO));

  /// The interface definition for internal structure creation
  ///
  /// @param gctx the geometry context at the creation of the internal structure
  ///
  /// @return a consistent set of detector volume internals
  ExternalStructure construct(const GeometryContext& gctx) const final;

 private:
  /// configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Acts::Experimental
