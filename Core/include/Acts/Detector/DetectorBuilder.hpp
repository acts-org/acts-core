// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Detector/interface/IDetectorBuilder.hpp"
#include "Acts/Detector/interface/IDetectorComponentBuilder.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <string>

namespace Acts {

class IMaterialDecorator;

namespace Experimental {

class IGeometryIdGenerator;

/// @brief Standard generic Detector builder that calls
/// the top level component builder and transfers the
/// result into a detector object
///
/// @note This is the last builder in the chain and the
/// the returned detector object is const and cannot be
/// modified anymore.
class DetectorBuilder final : public IDetectorBuilder {
 public:
  /// Nested configuration object
  struct Config {
    /// The name of the volume to be built
    std::string name = "unnamed";
    /// An external builder
    std::shared_ptr<const IDetectorComponentBuilder> builder = nullptr;
    /// A geometry id generator
    std::shared_ptr<const IGeometryIdGenerator> geoIdGenerator = nullptr;
    /// A material decorator
    std::shared_ptr<const IMaterialDecorator> materialDecorator = nullptr;
    /// Auxiliary information
    std::string auxiliary = "";
  };

  /// Constructor with configuration arguments
  ///
  /// @param cfg is the configuration struct
  /// @param mlogger logging instance for screen output
  DetectorBuilder(const Config& cfg,
                  std::unique_ptr<const Logger> mlogger =
                      getDefaultLogger("DetectorBuilder", Logging::INFO));

  /// Final implementation of a volume builder that is purely defined
  /// by an internal and external structure builder
  ///
  /// @param gctx The geometry context for this call
  ///
  /// @return an outgoing detector component
  std::shared_ptr<const Detector> construct(
      const GeometryContext& gctx) const final;

 private:
  /// configuration object
  Config m_cfg;

  /// Private access method to the logger
  const Logger& logger() const { return *m_logger; }

  /// logging instance
  std::unique_ptr<const Logger> m_logger;
};

}  // namespace Experimental
}  // namespace Acts
