// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Geant4Detector/Geant4Detector.hpp"

#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/KDTreeTrackingGeometryBuilder.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"
#include "Acts/Plugins/Geant4/Geant4DetectorElement.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <ostream>
#include <stdexcept>

#include "G4Transform3D.hh"
#include "G4VPhysicalVolume.hh"

namespace ActsExamples {

Geant4Detector::Geant4Detector(const Config& cfg)
    : Detector(Acts::getDefaultLogger("Geant4Detector", cfg.logLevel)),
      m_cfg(cfg) {
  if (m_cfg.g4World == nullptr) {
    throw std::invalid_argument(
        "Geant4Detector: no world Geant4 volume provided");
  }

  ACTS_INFO("Building an Acts::TrackingGeometry called '"
            << m_cfg.name << "' from the Geant4PhysVolume '"
            << m_cfg.g4World->GetName() << "'");

  m_nominalGeometryContext = Acts::GeometryContext();

  auto [surfaces, elements] = buildGeant4Volumes(cfg, logger());

  // Surface array creator
  auto surfaceArrayCreator = std::make_shared<const Acts::SurfaceArrayCreator>(
      Acts::SurfaceArrayCreator::Config(),
      logger().clone("SurfaceArrayCreator"));
  // Layer Creator
  Acts::LayerCreator::Config lcConfig;
  lcConfig.surfaceArrayCreator = surfaceArrayCreator;
  auto layerCreator = std::make_shared<Acts::LayerCreator>(
      lcConfig, logger().clone("LayerCreator"));
  // Layer array creator
  Acts::LayerArrayCreator::Config lacConfig;
  auto layerArrayCreator = std::make_shared<const Acts::LayerArrayCreator>(
      lacConfig, logger().clone("LayerArrayCreator"));
  // Tracking volume array creator
  Acts::TrackingVolumeArrayCreator::Config tvacConfig;
  auto tVolumeArrayCreator =
      std::make_shared<const Acts::TrackingVolumeArrayCreator>(
          tvacConfig, logger().clone("TrackingVolumeArrayCreator"));
  // configure the cylinder volume helper
  Acts::CylinderVolumeHelper::Config cvhConfig;
  cvhConfig.layerArrayCreator = layerArrayCreator;
  cvhConfig.trackingVolumeArrayCreator = tVolumeArrayCreator;
  auto cylinderVolumeHelper =
      std::make_shared<const Acts::CylinderVolumeHelper>(
          cvhConfig, logger().clone("CylinderVolumeHelper"));

  // Configure the tracking geometry builder, copy the surfaces in
  Acts::KDTreeTrackingGeometryBuilder::Config kdtCfg;
  kdtCfg.surfaces = surfaces;
  kdtCfg.layerCreator = layerCreator;
  kdtCfg.trackingVolumeHelper = cylinderVolumeHelper;
  kdtCfg.protoDetector = m_cfg.protoDetector;
  kdtCfg.geometryIdentifierHook = m_cfg.geometryIdentifierHook;

  // The KDT tracking geometry builder
  auto kdtBuilder = Acts::KDTreeTrackingGeometryBuilder(
      kdtCfg, logger().clone("KDTreeTrackingGeometryBuilder"));

  m_trackingGeometry = kdtBuilder.trackingGeometry(m_nominalGeometryContext);
}

std::tuple<std::vector<std::shared_ptr<Acts::Surface>>,
           std::vector<std::shared_ptr<Acts::Geant4DetectorElement>>>
Geant4Detector::buildGeant4Volumes(const Config& cfg,
                                   const Acts::Logger& logger) {
  // Generate the surface cache
  Acts::Geant4DetectorSurfaceFactory::Cache g4SurfaceCache;
  G4Transform3D g4ToWorld;

  Acts::Geant4DetectorSurfaceFactory{}.construct(
      g4SurfaceCache, g4ToWorld, *cfg.g4World, cfg.g4SurfaceOptions);

  ACTS_INFO("Found " << g4SurfaceCache.matchedG4Volumes
                     << " matching  Geant4 Physical volumes.");
  ACTS_INFO("Found " << g4SurfaceCache.sensitiveSurfaces.size()
                     << " converted sensitive Geant4 Physical volumes.");
  ACTS_INFO("Found " << g4SurfaceCache.passiveSurfaces.size()
                     << " converted passive Geant4 Physical volumes.");
  ACTS_INFO("Found " << g4SurfaceCache.convertedMaterials
                     << " converted Geant4 Material slabs.");

  std::vector<std::shared_ptr<Acts::Surface>> surfaces;
  std::vector<std::shared_ptr<Acts::Geant4DetectorElement>> elements;

  // Reserve the right amount of surfaces
  surfaces.reserve(g4SurfaceCache.sensitiveSurfaces.size() +
                   g4SurfaceCache.passiveSurfaces.size());
  elements.reserve(g4SurfaceCache.sensitiveSurfaces.size());

  // Add the sensitive surfaces
  for (const auto& [e, s] : g4SurfaceCache.sensitiveSurfaces) {
    surfaces.push_back(s);
    elements.push_back(e);
  }
  // Add the passive surfaces
  surfaces.insert(surfaces.end(), g4SurfaceCache.passiveSurfaces.begin(),
                  g4SurfaceCache.passiveSurfaces.end());

  return {std::move(surfaces), std::move(elements)};
}

}  // namespace ActsExamples
