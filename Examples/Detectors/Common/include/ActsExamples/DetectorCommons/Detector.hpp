// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"

#include <memory>
#include <vector>

namespace Acts {
class GeometryContext;
class TrackingGeometry;
class DetectorElementBase;
class Logger;
namespace Experimental {
class Detector;
}  // namespace Experimental
}  // namespace Acts

namespace ActsExamples {
class IContextDecorator;
struct Geant4ConstructionOptions;
}  // namespace ActsExamples

class G4VUserDetectorConstruction;

namespace ActsExamples {

/// Base class for detector instances
class Detector {
 public:
  explicit Detector(std::unique_ptr<const Acts::Logger> logger);
  virtual ~Detector();

  virtual const Acts::GeometryContext& nominalGeometryContext() const;

  virtual std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry()
      const;
  virtual std::shared_ptr<Acts::Experimental::Detector> gen2Geometry() const;
  virtual std::vector<std::shared_ptr<IContextDecorator>> contextDecorators()
      const;

  /// Build the Geant4 detector construction
  /// @note This throws an exception if Geant4 is not enabled
  /// @param options The Geant4 construction options
  /// @return The Geant4 detector construction
  virtual std::unique_ptr<G4VUserDetectorConstruction>
  buildGeant4DetectorConstruction(
      const Geant4ConstructionOptions& options) const;

 protected:
  const Acts::Logger& logger() const;

  std::unique_ptr<const Acts::Logger> m_logger;

  Acts::GeometryContext m_nominalGeometryContext;
  std::shared_ptr<const Acts::TrackingGeometry> m_trackingGeometry;
  std::shared_ptr<Acts::Experimental::Detector> m_gen2Geometry;
  std::vector<std::shared_ptr<const Acts::DetectorElementBase>> m_detectorStore;
  std::vector<std::shared_ptr<IContextDecorator>> m_contextDecorators;
};

}  // namespace ActsExamples
