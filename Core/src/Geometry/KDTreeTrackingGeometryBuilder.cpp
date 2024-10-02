// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/KDTreeTrackingGeometryBuilder.hpp"

#include "Acts/Geometry/CylinderLayer.hpp"
#include "Acts/Geometry/DiscLayer.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/ITrackingVolumeHelper.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/LayerCreator.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/RangeXD.hpp"

#include <cstddef>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <utility>

Acts::KDTreeTrackingGeometryBuilder::KDTreeTrackingGeometryBuilder(
    const Acts::KDTreeTrackingGeometryBuilder::Config& cfg,
    std::unique_ptr<const Logger> logger)
    : m_cfg(cfg), m_logger(std::move(logger)) {
  m_cfg.protoDetector.harmonize();
}

std::unique_ptr<const Acts::TrackingGeometry>
Acts::KDTreeTrackingGeometryBuilder::trackingGeometry(
    const GeometryContext& gctx) const {
  using MeasuredSurface =
      std::pair<std::array<ActsScalar, 2u>, std::shared_ptr<Surface>>;
  // Prepare all the surfaces
  std::vector<MeasuredSurface> surfacesMeasured;
  surfacesMeasured.reserve(m_cfg.surfaces.size());
  for (auto& s : m_cfg.surfaces) {
    auto ext = s->polyhedronRepresentation(gctx, 1u).extent();
    surfacesMeasured.push_back(MeasuredSurface{
        std::array<ActsScalar, 2u>{ext.medium(BinningValue::binZ),
                                   ext.medium(BinningValue::binR)},
        s});
  }

  // Create the KDTree
  ACTS_INFO("Full KDTree has " << surfacesMeasured.size() << " surfaces.");
  SurfaceKDT surfaceKDT(std::move(surfacesMeasured));

  // Walk through the proto builder
  auto protoWorld = m_cfg.protoDetector.worldVolume;

  KDTreeTrackingGeometryBuilder::Cache cCache;

  auto worldTrackingVolume =
      translateVolume(cCache, gctx, surfaceKDT, protoWorld);

  ACTS_INFO("Retrieved " << cCache.surfaceCounter
                         << " surfaces from the KDTree.");

  // return the geometry to the service
  return std::make_unique<const Acts::TrackingGeometry>(worldTrackingVolume);
}

std::shared_ptr<Acts::TrackingVolume>
Acts::KDTreeTrackingGeometryBuilder::translateVolume(
    Cache& cCache, const GeometryContext& gctx, const SurfaceKDT& kdt,
    const ProtoVolume& ptVolume, const std::string& indent) const {
  ACTS_DEBUG(indent << "Processing ProtoVolume: " << ptVolume.name);
  std::vector<std::shared_ptr<const TrackingVolume>> translatedVolumes = {};

  // Volume extent
  auto rangeR = ptVolume.extent.range(Acts::BinningValue::binR);
  auto rangeZ = ptVolume.extent.range(Acts::BinningValue::binZ);

  // Simple gap volume
  if (!ptVolume.container.has_value()) {
    ACTS_VERBOSE(indent << "> empty volume to be built");
    MutableTrackingVolumeVector mtv = {};
    auto tVolume = m_cfg.trackingVolumeHelper->createGapTrackingVolume(
        gctx, mtv, nullptr, rangeR.min(), rangeR.max(), rangeZ.min(),
        rangeZ.max(), 0, false, ptVolume.name);
    ACTS_DEBUG(indent << "> translated into gap volume bounds: "
                      << tVolume->volumeBounds());
    return tVolume;
  } else {
    // Get the container information
    auto& cts = ptVolume.container.value();

    // Container information must be present
    if (cts.constituentVolumes.empty()) {
      throw std::invalid_argument(
          "KDTreeTrackingGeometryBuilder: no constituents given.");
    }

    // This volume is a volume container
    if (!cts.layerContainer) {
      ACTS_VERBOSE(indent << "> volume container with "
                          << cts.constituentVolumes.size() << " constituents.");
      for (auto& cVolume : cts.constituentVolumes) {
        auto dtVolume = translateVolume(cCache, gctx, kdt, cVolume,
                                        indent + m_cfg.hierarchyIndent);
        translatedVolumes.push_back(dtVolume);
      }
      auto tVolume = m_cfg.trackingVolumeHelper->createContainerTrackingVolume(
          gctx, translatedVolumes);
      ACTS_DEBUG(indent << "> translated into container volume bounds: "
                        << tVolume->volumeBounds());
      return tVolume;
    } else {
      // This volume is a layer container
      std::vector<std::shared_ptr<const Layer>> layers = {};
      ACTS_VERBOSE(indent << "> layer container with "
                          << cts.constituentVolumes.size() << " layers.");
      for (auto& plVolume : cts.constituentVolumes) {
        if (plVolume.internal.has_value()) {
          layers.push_back(translateLayer(cCache, gctx, kdt, plVolume,
                                          indent + m_cfg.hierarchyIndent));
        } else {
          ACTS_WARNING(indent << "> layer type volume has no internal "
                                 "description, layer not built.");
        }
      }
      // Create a new tracking volume with those layers
      auto tVolume = m_cfg.trackingVolumeHelper->createTrackingVolume(
          gctx, layers, {}, nullptr, rangeR.min(), rangeR.max(), rangeZ.min(),
          rangeZ.max(), ptVolume.name);
      ACTS_DEBUG(indent << "> translated into bounds: "
                        << tVolume->volumeBounds());
      return tVolume;
    }
  }
}

/// @return a new tracking volume
std::shared_ptr<const Acts::Layer>
Acts::KDTreeTrackingGeometryBuilder::translateLayer(
    Cache& cCache, const GeometryContext& gctx, const SurfaceKDT& kdt,
    const ProtoVolume& plVolume, const std::string& indent) const {
  ACTS_DEBUG(indent + "Processing ProtoVolume: " << plVolume.name);

  // This is only called if the volume has internal structure
  auto& its = plVolume.internal.value();

  // Try to pull from the kd tree
  RangeXD<2u, ActsScalar> zrRange;
  zrRange[0u] = plVolume.extent.range(Acts::BinningValue::binZ);
  zrRange[1u] = plVolume.extent.range(Acts::BinningValue::binR);

  auto layerSurfaces = kdt.rangeSearchWithKey(zrRange);
  ACTS_VERBOSE(indent + ">> looking z/r range = " << zrRange.toString());
  ACTS_VERBOSE(indent + ">> found " << layerSurfaces.size()
                                    << " surfaces in the KDTree.");
  cCache.surfaceCounter += layerSurfaces.size();

  std::shared_ptr<const Acts::Layer> tLayer = nullptr;

  if (layerSurfaces.size() == 1u) {
    auto surface = layerSurfaces[0u].second;
    const auto& transform = surface->transform(gctx);
    if (its.layerType == Acts::Surface::SurfaceType::Cylinder) {
      ACTS_VERBOSE(indent +
                   ">> creating cylinder layer from a single surface.");
      // Get the bounds
      auto cylinderBounds =
          dynamic_cast<const CylinderBounds*>(&(surface->bounds()));
      auto cylinderBoundsClone =
          std::make_shared<const CylinderBounds>(*cylinderBounds);
      auto cylinderLayer =
          CylinderLayer::create(transform, cylinderBoundsClone, nullptr, 1.);
      cylinderLayer->assignSurfaceMaterial(surface->surfaceMaterialSharedPtr());
      tLayer = cylinderLayer;
    } else if (its.layerType == Acts::Surface::SurfaceType::Disc) {
      ACTS_VERBOSE(indent +
                   ">> creating cylinder layer from a single surface.");
      // Get the bounds
      auto radialBounds =
          dynamic_cast<const RadialBounds*>(&(surface->bounds()));
      auto radialBoundsClone =
          std::make_shared<const RadialBounds>(*radialBounds);
      auto discLayer =
          DiscLayer::create(transform, radialBoundsClone, nullptr, 1.);
      discLayer->assignSurfaceMaterial(surface->surfaceMaterialSharedPtr());
      tLayer = discLayer;
    } else {
      throw std::invalid_argument(
          "KDTreeTrackingGeometryBuilder: layer type is neither cylinder nor "
          "disk.");
    }

  } else if (layerSurfaces.size() > 1u) {
    // Make a const collection out of the surfaces
    std::vector<std::shared_ptr<const Surface>> cLayerSurfaces;
    cLayerSurfaces.reserve(layerSurfaces.size());
    for (const auto& s : layerSurfaces) {
      cLayerSurfaces.push_back(s.second);
    }

    Acts::BinningType bType0 = Acts::equidistant;
    Acts::BinningType bType1 = Acts::equidistant;
    std::size_t bins0 = 0;
    std::size_t bins1 = 0;
    // In case explicit binning is given
    if (its.surfaceBinning.size() == 2u) {
      bType0 = its.surfaceBinning[0u].type;
      bType1 = its.surfaceBinning[1u].type;
      // In case explicit bin numbers are given in addition
      if (bType0 == Acts::equidistant && bType1 == Acts::equidistant &&
          its.surfaceBinning[0u].bins() > 1u &&
          its.surfaceBinning[1u].bins() > 1u) {
        bins0 = its.surfaceBinning[0u].bins();
        bins1 = its.surfaceBinning[1u].bins();
        ACTS_VERBOSE(indent + ">> binning provided externally to be "
                     << bins0 << " x " << bins1 << ".");
      }
    }

    Acts::ProtoLayer pLayer(gctx, cLayerSurfaces);
    pLayer.envelope = plVolume.extent.envelope();
    if (its.layerType == Acts::Surface::SurfaceType::Cylinder) {
      ACTS_VERBOSE(indent + ">> creating cylinder layer with "
                   << cLayerSurfaces.size() << " surfaces.");
      // Forced equidistant or auto-binned
      tLayer = (bins0 * bins1 > 0)
                   ? m_cfg.layerCreator->cylinderLayer(gctx, cLayerSurfaces,
                                                       bins0, bins1, pLayer)
                   : m_cfg.layerCreator->cylinderLayer(gctx, cLayerSurfaces,
                                                       bType0, bType1, pLayer);

    } else if (its.layerType == Acts::Surface::SurfaceType::Disc) {
      ACTS_VERBOSE(indent + ">> creating disc layer with "
                   << cLayerSurfaces.size() << " surfaces.");
      // Forced equidistant or auto-binned
      tLayer = (bins0 * bins1 > 0)
                   ? m_cfg.layerCreator->discLayer(gctx, cLayerSurfaces, bins0,
                                                   bins1, pLayer)

                   : m_cfg.layerCreator->discLayer(gctx, cLayerSurfaces, bType0,
                                                   bType1, pLayer);
    } else {
      throw std::invalid_argument(
          "KDTreeTrackingGeometryBuilder: layer type is neither cylinder nor "
          "disk.");
    }
  }
  if (tLayer != nullptr && tLayer->representingVolume() != nullptr) {
    ACTS_DEBUG(indent << "> translated into layer bounds: "
                      << tLayer->representingVolume()->volumeBounds());
  } else {
    throw std::runtime_error(
        "KDTreeTrackingGeometryBuilder: layer was not built.");
  }

  return tLayer;
}
