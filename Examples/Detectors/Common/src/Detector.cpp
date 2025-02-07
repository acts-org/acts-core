// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/DetectorCommons/Detector.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/IContextDecorator.hpp"

namespace ActsExamples {

Detector::Detector(std::unique_ptr<const Acts::Logger> logger)
    : m_logger(std::move(logger)) {}

Detector::~Detector() = default;

std::vector<std::shared_ptr<IContextDecorator>> Detector::contextDecorators()
    const {
  return m_contextDecorators;
}

std::unique_ptr<G4VUserDetectorConstruction>
Detector::buildGeant4DetectorConstruction(
    const Geant4ConstructionOptions& /*options*/) const {
  throw std::runtime_error("Geant4 detector construction is not available.");
}

const Acts::GeometryContext& Detector::nominalGeometryContext() const {
  return m_nominalGeometryContext;
}

std::shared_ptr<const Acts::TrackingGeometry> Detector::trackingGeometry()
    const {
  if (m_trackingGeometry == nullptr) {
    throw std::runtime_error("Tracking geometry is not built");
  }
  return m_trackingGeometry;
}

std::shared_ptr<Acts::Experimental::Detector> Detector::gen2Geometry() const {
  if (m_gen2Geometry == nullptr) {
    throw std::runtime_error("Gen2 geometry is not built");
  }
  return m_gen2Geometry;
}

const Acts::Logger& Detector::logger() const {
  return *m_logger;
}

}  // namespace ActsExamples
