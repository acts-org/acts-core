// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Detector/Detector.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/DD4hep/DD4hepDetectorElement.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>
#include <tuple>

namespace dd4hep {
class DetElement;
}

namespace Acts {

class IMaterialDecorator;

namespace Experimental {

/// @brief This class allows to generate layer structure builders for dd4hep sub detectors
/// It performs an intermediate step by taking dd4hep::DetElemnent objects that
/// describe a detector structure and prepares the translation of the detector
/// element and eventual passive surfaces.
///
/// It would also build passive support structures if configured to do so.
///
class DD4hepDetectorStructure {
 public:
  /// @brief  Nested options struct for the building
  struct Options {
    /// The log level of the tools
    Logging::Level logLevel = Logging::INFO;
    /// If this string is not empty, the detector is not built, but only
    /// a building graph is generated as `.dot` file with the given name
    std::string emulateToGraph = "";
    /// A Top level geometry id generator
    std::shared_ptr<const IGeometryIdGenerator> geoIdGenerator = nullptr;
    /// A Top level material decorator
    std::shared_ptr<const IMaterialDecorator> materialDecorator = nullptr;
  };

  /// Constructor with from file name
  ///
  /// @param logger is the screen output logger
  ///
  /// @note this needs to be provided
  explicit DD4hepDetectorStructure(std::unique_ptr<const Logger> logger =
                                       getDefaultLogger("DD4hepLayerStructure",
                                                        Acts::Logging::INFO));

  DD4hepDetectorStructure() = delete;

  /// @brief This method builds a DD4hep detector
  ///
  /// @param gctx the geometry context
  /// @param dd4hepElement is the dd4hep::DetElement of the world volume
  /// @param options is the options struct
  ///
  /// @note the lifetime of the detector elements in the returned store
  /// has to exceed the lifetime of the detector for memory management
  /// reasons.
  ///
  /// @return a detector, and the detector element store
  std::tuple<std::shared_ptr<const Detector>, DD4hepDetectorElement::Store>
  construct(const GeometryContext& gctx,
            const dd4hep::DetElement& dd4hepElement,
            const Options& options) const;

 private:
  /// Logging instance
  std::unique_ptr<const Logger> m_logger;

  /// Private access to the logger
  const Logger& logger() const { return *m_logger; }
};

}  // namespace Experimental
}  // namespace Acts
