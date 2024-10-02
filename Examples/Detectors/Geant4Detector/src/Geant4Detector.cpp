// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Geant4Detector/Geant4Detector.hpp"

#include "Acts/Geometry/CylinderVolumeHelper.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/KDTreeTrackingGeometryBuilder.hpp"
#include "Acts/Geometry/LayerArrayCreator.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/SurfaceArrayCreator.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolumeArrayCreator.hpp"

#include <ostream>
#include <stdexcept>

#include "G4Transform3D.hh"
#include "G4VPhysicalVolume.hh"

auto ActsExamples::Geant4::Geant4Detector::constructDetector(
    const ActsExamples::Geant4::Geant4Detector::Config& cfg,
    const Acts::Logger& logger)
    -> std::tuple<DetectorPtr, ContextDecorators, DetectorElements> {
  if (cfg.g4World == nullptr) {
    throw std::invalid_argument(
        "Geant4Detector: no world Geant4 volume provided");
  }

  ACTS_INFO("Building an Acts::Detector called '"
            << cfg.name << "' from the Geant4PhysVolume '"
            << cfg.g4World->GetName() << "'");

  DetectorPtr detector = nullptr;
  ContextDecorators decorators = {};

  auto [surfaces, elements] = convertGeant4Volumes(cfg, logger);

  return std::tie(detector, decorators, elements);
}

auto ActsExamples::Geant4::Geant4Detector::constructTrackingGeometry(
    const ActsExamples::Geant4::Geant4Detector::Config& cfg,
    const Acts::Logger& logger)
    -> std::tuple<TrackingGeometryPtr, ContextDecorators, DetectorElements> {
  if (cfg.g4World == nullptr) {
    throw std::invalid_argument(
        "Geant4Detector: no world Geant4 volume provided");
  }

  ACTS_INFO("Building an Acts::TrackingGeometry called '"
            << cfg.name << "' from the Geant4PhysVolume '"
            << cfg.g4World->GetName() << "'");

  ContextDecorators decorators = {};

  auto [surfaces, elements] = convertGeant4Volumes(cfg, logger);

  // Surface array creator
  auto surfaceArrayCreator = std::make_shared<const Acts::SurfaceArrayCreator>(
      Acts::SurfaceArrayCreator::Config(), logger.clone("SurfaceArrayCreator"));
  // Layer Creator
  Acts::LayerCreator::Config lcConfig;
  lcConfig.surfaceArrayCreator = surfaceArrayCreator;
  auto layerCreator = std::make_shared<Acts::LayerCreator>(
      lcConfig, logger.clone("LayerCreator"));
  // Layer array creator
  Acts::LayerArrayCreator::Config lacConfig;
  auto layerArrayCreator = std::make_shared<const Acts::LayerArrayCreator>(
      lacConfig, logger.clone("LayerArrayCreator"));
  // Tracking volume array creator
  Acts::TrackingVolumeArrayCreator::Config tvacConfig;
  auto tVolumeArrayCreator =
      std::make_shared<const Acts::TrackingVolumeArrayCreator>(
          tvacConfig, logger.clone("TrackingVolumeArrayCreator"));
  // configure the cylinder volume helper
  Acts::CylinderVolumeHelper::Config cvhConfig;
  cvhConfig.layerArrayCreator = layerArrayCreator;
  cvhConfig.trackingVolumeArrayCreator = tVolumeArrayCreator;
  auto cylinderVolumeHelper =
      std::make_shared<const Acts::CylinderVolumeHelper>(
          cvhConfig, logger.clone("CylinderVolumeHelper"));

  // Configure the tracking geometry builder, copy the surfaces in
  Acts::KDTreeTrackingGeometryBuilder::Config kdtCfg;
  kdtCfg.surfaces = surfaces;
  kdtCfg.layerCreator = layerCreator;
  kdtCfg.trackingVolumeHelper = cylinderVolumeHelper;
  kdtCfg.protoDetector = cfg.protoDetector;
  kdtCfg.geometryIdentifierHook = cfg.geometryIdentifierHook;

  // The KDT tracking geometry builder
  auto kdtBuilder = Acts::KDTreeTrackingGeometryBuilder(
      kdtCfg, logger.clone("KDTreeTrackingGeometryBuilder"));

  Acts::GeometryContext tContext;
  TrackingGeometryPtr trackingGeometry = kdtBuilder.trackingGeometry(tContext);

  return std::tie(trackingGeometry, decorators, elements);
}

auto ActsExamples::Geant4::Geant4Detector::convertGeant4Volumes(
    const Geant4Detector::Config& cfg, const Acts::Logger& logger) const
    -> std::tuple<ActsExamples::Geant4::Geant4Detector::Surfaces,
                  ActsExamples::Geant4::Geant4Detector::DetectorElements> {
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

  Surfaces surfaces = {};
  DetectorElements elements = {};

  // Reserve the right amount of surfaces
  surfaces.reserve(g4SurfaceCache.sensitiveSurfaces.size() +
                   g4SurfaceCache.passiveSurfaces.size());
  elements.reserve(g4SurfaceCache.sensitiveSurfaces.size());

  // Add the sensitive surfaces
  for (const auto& [e, s] : g4SurfaceCache.sensitiveSurfaces) {
    elements.push_back(e);
    surfaces.push_back(s);
  }
  // Add the passive surfaces
  surfaces.insert(surfaces.end(), g4SurfaceCache.passiveSurfaces.begin(),
                  g4SurfaceCache.passiveSurfaces.end());

  return std::tie(surfaces, elements);
}
