// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Json/JsonSurfacesWriter.hpp"

#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Plugins/Json/GeometryHierarchyMapJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Plugins/Json/VolumeJsonConverter.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Utilities/Paths.hpp"

#include <cstddef>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

using namespace ActsExamples;

JsonSurfacesWriter::JsonSurfacesWriter(const JsonSurfacesWriter::Config& config,
                                       Acts::Logging::Level level)
    : m_cfg(config),
      m_logger(Acts::getDefaultLogger("JsonSurfacesWriter", level)) {
  if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument("Missing tracking geometry");
  }
  m_world = m_cfg.trackingGeometry->highestTrackingVolume();
  if (m_world == nullptr) {
    throw std::invalid_argument("Could not identify the world volume");
  }
}

std::string JsonSurfacesWriter::name() const {
  return "JsonSurfacesWriter";
}

namespace {

using SurfaceContainer =
    Acts::GeometryHierarchyMap<std::shared_ptr<const Acts::Surface>>;
using SurfaceConverter = Acts::GeometryHierarchyMapJsonConverter<
    std::shared_ptr<const Acts::Surface>>;

/// Write all child surfaces and descend into confined volumes.
void collectSurfaces(std::vector<SurfaceContainer::InputElement>& cSurfaces,
                     const Acts::TrackingVolume& volume, bool writeLayer,
                     bool writeApproach, bool writeSensitive,
                     bool writeBoundary) {
  // Process all layers that are directly stored within this volume
  if (volume.confinedLayers() != nullptr) {
    for (const auto& layer : volume.confinedLayers()->arrayObjects()) {
      // We jump navigation layers
      if (layer->layerType() == Acts::navigation) {
        continue;
      }
      // Layer surface
      if (writeLayer) {
        auto layerSurfacePtr = layer->surfaceRepresentation().getSharedPtr();
        cSurfaces.push_back(SurfaceContainer::InputElement{
            layer->surfaceRepresentation().geometryId(), layerSurfacePtr});
      }
      // Approach surfaces
      if (writeApproach && layer->approachDescriptor() != nullptr) {
        for (auto sf : layer->approachDescriptor()->containedSurfaces()) {
          cSurfaces.push_back(SurfaceContainer::InputElement{
              sf->geometryId(), sf->getSharedPtr()});
        }
      }
      // Check for sensitive surfaces
      if (layer->surfaceArray() != nullptr && writeSensitive) {
        for (const auto& surface : layer->surfaceArray()->surfaces()) {
          if (surface != nullptr) {
            cSurfaces.push_back(SurfaceContainer::InputElement{
                surface->geometryId(), surface->getSharedPtr()});
          }
        }
      }
    }
    // This is a navigation volume, write the boundaries
    if (writeBoundary) {
      for (const auto& bsurface : volume.boundarySurfaces()) {
        const auto& bsRep = bsurface->surfaceRepresentation();
        cSurfaces.push_back(SurfaceContainer::InputElement{
            bsRep.geometryId(), bsRep.getSharedPtr()});
      }
    }
  }
  // Step down into hierarchy to process all child volumnes
  if (volume.confinedVolumes()) {
    for (const auto& confined : volume.confinedVolumes()->arrayObjects()) {
      collectSurfaces(cSurfaces, *confined.get(), writeLayer, writeApproach,
                      writeSensitive, writeBoundary);
    }
  }
}
}  // namespace

ProcessCode JsonSurfacesWriter::write(const AlgorithmContext& ctx) {
  if (!m_cfg.writePerEvent) {
    return ProcessCode::SUCCESS;
  }

  std::ofstream out;
  out.open(perEventFilepath(m_cfg.outputDir, "detector.json", ctx.eventNumber));

  std::vector<SurfaceContainer::InputElement> cSurfaces;
  collectSurfaces(cSurfaces, *m_world, m_cfg.writeLayer, m_cfg.writeApproach,
                  m_cfg.writeSensitive, m_cfg.writeBoundary);
  SurfaceContainer sContainer(cSurfaces);

  if (!m_cfg.writeOnlyNames) {
    auto j = SurfaceConverter("surfaces").toJson(sContainer, nullptr);
    out << std::setprecision(m_cfg.outputPrecision) << j.dump(2);
    out.close();
  } else {
    using NamedContainer = Acts::GeometryHierarchyMap<std::string>;
    using NamedConverter = Acts::GeometryHierarchyMapJsonConverter<std::string>;

    std::vector<std::pair<Acts::GeometryIdentifier, std::string>> namedEntries;
    for (std::size_t is = 0; is < sContainer.size(); ++is) {
      Acts::GeometryIdentifier geometryId = sContainer.idAt(is);
      std::stringstream geoTypeName;
      geoTypeName << geometryId;
      namedEntries.push_back({geometryId, geoTypeName.str()});
    }
    NamedContainer nContainer(namedEntries);
    auto j = NamedConverter("surface_types").toJson(nContainer, nullptr);
    out << j.dump(2);
    out.close();
  }

  return ProcessCode::SUCCESS;
}

ProcessCode JsonSurfacesWriter::finalize() {
  std::ofstream out;
  out.open(joinPaths(m_cfg.outputDir, "detector.csv"));

  std::vector<SurfaceContainer::InputElement> cSurfaces;
  collectSurfaces(cSurfaces, *m_world, m_cfg.writeLayer, m_cfg.writeApproach,
                  m_cfg.writeSensitive, m_cfg.writeBoundary);
  SurfaceContainer sContainer(cSurfaces);

  auto j = SurfaceConverter("surfaces").toJson(sContainer, nullptr);
  out << std::setprecision(m_cfg.outputPrecision) << j.dump(2);
  out.close();

  return ProcessCode::SUCCESS;
}
