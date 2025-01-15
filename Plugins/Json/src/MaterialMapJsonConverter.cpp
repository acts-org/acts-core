// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/MaterialMapJsonConverter.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/CuboidVolumeBounds.hpp"
#include "Acts/Geometry/CutoutCylinderVolumeBounds.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/ProtoSurfaceMaterial.hpp"
#include "Acts/Material/ProtoVolumeMaterial.hpp"
#include "Acts/Plugins/Json/ITrackingGeometryJsonDecorator.hpp"
#include "Acts/Plugins/Json/IVolumeMaterialJsonDecorator.hpp"
#include "Acts/Plugins/Json/MaterialJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Plugins/Json/VolumeJsonConverter.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include <Acts/Surfaces/AnnulusBounds.hpp>
#include <Acts/Surfaces/CylinderBounds.hpp>
#include <Acts/Surfaces/RadialBounds.hpp>
#include <Acts/Surfaces/SurfaceBounds.hpp>
#include <Acts/Surfaces/TrapezoidBounds.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <numbers>
#include <stdexcept>

namespace Acts {
// specialisations of decoration helper function
// to pick correct objects from the container object
template <>
inline void decorateJson<Acts::SurfaceAndMaterialWithContext>(
    const ITrackingGeometryJsonDecorator* decorator,
    const Acts::SurfaceAndMaterialWithContext& src, nlohmann::json& dest) {
  if (decorator != nullptr && std::get<0>(src) != nullptr) {
    decorator->decorate(*std::get<0>(src), dest);
  }
}
template <>
inline void decorateJson<Acts::TrackingVolumeAndMaterial>(
    const ITrackingGeometryJsonDecorator* decorator,
    const Acts::TrackingVolumeAndMaterial& src, nlohmann::json& dest) {
  if (decorator != nullptr && src.first != nullptr) {
    decorator->decorate(*src.first, dest);
  }
}

template <>
inline void decorateJson<Acts::IVolumeMaterial>(
    const IVolumeMaterialJsonDecorator* decorator,
    const Acts::IVolumeMaterial* src, nlohmann::json& dest) {
  if (decorator != nullptr && src != nullptr) {
    decorator->decorate(*src, dest);
  }
}
template <>
inline void decorateJson<Acts::ISurfaceMaterial>(
    const IVolumeMaterialJsonDecorator* decorator,
    const Acts::ISurfaceMaterial* src, nlohmann::json& dest) {
  if (decorator != nullptr && src != nullptr) {
    decorator->decorate(*src, dest);
  }
}
}  // namespace Acts

namespace {

Acts::SurfaceAndMaterialWithContext defaultSurfaceMaterial(
    const std::shared_ptr<const Acts::Surface>& surface,
    const Acts::GeometryContext& context) {
  if (surface->surfaceMaterialSharedPtr() != nullptr) {
    return {surface, surface->surfaceMaterialSharedPtr(), context};
  }
  Acts::BinUtility bUtility;
  // Check which type of bounds is associated to the surface
  const Acts::SurfaceBounds& surfaceBounds = surface->bounds();
  const Acts::RadialBounds* radialBounds =
      dynamic_cast<const Acts::RadialBounds*>(&surfaceBounds);
  const Acts::CylinderBounds* cylinderBounds =
      dynamic_cast<const Acts::CylinderBounds*>(&surfaceBounds);
  const Acts::AnnulusBounds* annulusBounds =
      dynamic_cast<const Acts::AnnulusBounds*>(&surfaceBounds);
  const Acts::RectangleBounds* rectangleBounds =
      dynamic_cast<const Acts::RectangleBounds*>(&surfaceBounds);
  const Acts::TrapezoidBounds* trapezoidBounds =
      dynamic_cast<const Acts::TrapezoidBounds*>(&surfaceBounds);

  if (radialBounds != nullptr) {
    bUtility += Acts::BinUtility(
        1,
        radialBounds->get(Acts::RadialBounds::eAveragePhi) -
            radialBounds->get(Acts::RadialBounds::eHalfPhiSector),
        radialBounds->get(Acts::RadialBounds::eAveragePhi) +
            radialBounds->get(Acts::RadialBounds::eHalfPhiSector),
        (radialBounds->get(Acts::RadialBounds::eHalfPhiSector) -
         std::numbers::pi) < Acts::s_epsilon
            ? Acts::closed
            : Acts::open,
        Acts::AxisDirection::AxisPhi);
    bUtility += Acts::BinUtility(1, radialBounds->rMin(), radialBounds->rMax(),
                                 Acts::open, Acts::AxisDirection::AxisR);
  }
  if (cylinderBounds != nullptr) {
    bUtility += Acts::BinUtility(
        1,
        cylinderBounds->get(Acts::CylinderBounds::eAveragePhi) -
            cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector),
        cylinderBounds->get(Acts::CylinderBounds::eAveragePhi) +
            cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector),
        (cylinderBounds->get(Acts::CylinderBounds::eHalfPhiSector) -
         std::numbers::pi) < Acts::s_epsilon
            ? Acts::closed
            : Acts::open,
        Acts::AxisDirection::AxisPhi);
    bUtility += Acts::BinUtility(
        1, -1 * cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ),
        cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ), Acts::open,
        Acts::AxisDirection::AxisZ);
  }
  if (annulusBounds != nullptr) {
    bUtility +=
        Acts::BinUtility(1, annulusBounds->get(Acts::AnnulusBounds::eMinPhiRel),
                         annulusBounds->get(Acts::AnnulusBounds::eMaxPhiRel),
                         Acts::open, Acts::AxisDirection::AxisPhi);
    bUtility += Acts::BinUtility(1, static_cast<float>(annulusBounds->rMin()),
                                 static_cast<float>(annulusBounds->rMax()),
                                 Acts::open, Acts::AxisDirection::AxisR);
  }
  if (rectangleBounds != nullptr) {
    bUtility +=
        Acts::BinUtility(1, rectangleBounds->get(Acts::RectangleBounds::eMinX),
                         rectangleBounds->get(Acts::RectangleBounds::eMaxX),
                         Acts::open, Acts::AxisDirection::AxisX);
    bUtility +=
        Acts::BinUtility(1, rectangleBounds->get(Acts::RectangleBounds::eMinY),
                         rectangleBounds->get(Acts::RectangleBounds::eMaxY),
                         Acts::open, Acts::AxisDirection::AxisY);
  }
  if (trapezoidBounds != nullptr) {
    double halfLengthX =
        std::max(trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthXnegY),
                 trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthXposY));
    bUtility += Acts::BinUtility(1, -1 * halfLengthX, halfLengthX, Acts::open,
                                 Acts::AxisDirection::AxisX);
    bUtility += Acts::BinUtility(
        1, -1 * trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthY),
        trapezoidBounds->get(Acts::TrapezoidBounds::eHalfLengthY), Acts::open,
        Acts::AxisDirection::AxisY);
  }
  return {surface, std::make_shared<Acts::ProtoSurfaceMaterial>(bUtility),
          context};
}

Acts::TrackingVolumeAndMaterial defaultVolumeMaterial(
    const Acts::TrackingVolume* volume) {
  Acts::BinUtility bUtility;
  if (volume->volumeMaterialPtr() != nullptr) {
    return {volume, volume->volumeMaterialPtr()};
  }
  // Check which type of bound is associated to the volume
  auto cyBounds = dynamic_cast<const Acts::CylinderVolumeBounds*>(
      &(volume->volumeBounds()));
  auto cutcylBounds = dynamic_cast<const Acts::CutoutCylinderVolumeBounds*>(
      &(volume->volumeBounds()));
  auto cuBounds =
      dynamic_cast<const Acts::CuboidVolumeBounds*>(&(volume->volumeBounds()));

  if (cyBounds != nullptr) {
    bUtility +=
        Acts::BinUtility(1, cyBounds->get(Acts::CylinderVolumeBounds::eMinR),
                         cyBounds->get(Acts::CylinderVolumeBounds::eMaxR),
                         Acts::open, Acts::AxisDirection::AxisR);
    bUtility += Acts::BinUtility(
        1, -cyBounds->get(Acts::CylinderVolumeBounds::eHalfPhiSector),
        cyBounds->get(Acts::CylinderVolumeBounds::eHalfPhiSector),
        (cyBounds->get(Acts::CylinderVolumeBounds::eHalfPhiSector) -
         std::numbers::pi) < Acts::s_epsilon
            ? Acts::closed
            : Acts::open,
        Acts::AxisDirection::AxisPhi);
    bUtility += Acts::BinUtility(
        1, -cyBounds->get(Acts::CylinderVolumeBounds::eHalfLengthZ),
        cyBounds->get(Acts::CylinderVolumeBounds::eHalfLengthZ), Acts::open,
        Acts::AxisDirection::AxisZ);
  }
  if (cutcylBounds != nullptr) {
    bUtility += Acts::BinUtility(
        1, cutcylBounds->get(Acts::CutoutCylinderVolumeBounds::eMinR),
        cutcylBounds->get(Acts::CutoutCylinderVolumeBounds::eMaxR), Acts::open,
        Acts::AxisDirection::AxisR);
    bUtility += Acts::BinUtility(1, -std::numbers::pi_v<float>,
                                 std::numbers::pi_v<float>, Acts::closed,
                                 Acts::AxisDirection::AxisPhi);
    bUtility += Acts::BinUtility(
        1, -cutcylBounds->get(Acts::CutoutCylinderVolumeBounds::eHalfLengthZ),
        cutcylBounds->get(Acts::CutoutCylinderVolumeBounds::eHalfLengthZ),
        Acts::open, Acts::AxisDirection::AxisZ);
  } else if (cuBounds != nullptr) {
    bUtility += Acts::BinUtility(
        1, -cuBounds->get(Acts::CuboidVolumeBounds::eHalfLengthX),
        cuBounds->get(Acts::CuboidVolumeBounds::eHalfLengthX), Acts::open,
        Acts::AxisDirection::AxisX);
    bUtility += Acts::BinUtility(
        1, -cuBounds->get(Acts::CuboidVolumeBounds::eHalfLengthY),
        cuBounds->get(Acts::CuboidVolumeBounds::eHalfLengthY), Acts::open,
        Acts::AxisDirection::AxisY);
    bUtility += Acts::BinUtility(
        1, -cuBounds->get(Acts::CuboidVolumeBounds::eHalfLengthZ),
        cuBounds->get(Acts::CuboidVolumeBounds::eHalfLengthZ), Acts::open,
        Acts::AxisDirection::AxisZ);
  }
  return {volume, std::make_shared<Acts::ProtoVolumeMaterial>(bUtility)};
}
}  // namespace

Acts::MaterialMapJsonConverter::MaterialMapJsonConverter(
    const Acts::MaterialMapJsonConverter::Config& config,
    Acts::Logging::Level level)
    : m_cfg(config),
      m_logger{getDefaultLogger("MaterialMapJsonConverter", level)},
      m_volumeMaterialConverter(m_volumeName),
      m_volumeConverter(m_volumeName),
      m_surfaceMaterialConverter(m_surfaceName),
      m_surfaceConverter(m_surfaceName) {}

/// Convert method
///
nlohmann::json Acts::MaterialMapJsonConverter::materialMapsToJson(
    const DetectorMaterialMaps& maps,
    const IVolumeMaterialJsonDecorator* decorator) {
  VolumeMaterialMap volumeMap = maps.second;
  std::vector<std::pair<GeometryIdentifier, const IVolumeMaterial*>>
      mapVolumeInit;
  for (const auto& [key, value] : volumeMap) {
    mapVolumeInit.push_back({key, value.get()});
  }
  GeometryHierarchyMap<const IVolumeMaterial*> hierarchyVolumeMap(
      mapVolumeInit);
  nlohmann::json materialVolume =
      m_volumeMaterialConverter.toJson(hierarchyVolumeMap, decorator);
  SurfaceMaterialMap surfaceMap = maps.first;
  std::vector<std::pair<GeometryIdentifier, const ISurfaceMaterial*>>
      mapSurfaceInit;
  for (const auto& [key, value] : surfaceMap) {
    mapSurfaceInit.push_back({key, value.get()});
  }
  GeometryHierarchyMap<const ISurfaceMaterial*> hierarchySurfaceMap(
      mapSurfaceInit);
  nlohmann::json materialSurface =
      m_surfaceMaterialConverter.toJson(hierarchySurfaceMap, decorator);
  nlohmann::json materialMap;
  materialMap["Volumes"] = materialVolume;
  materialMap["Surfaces"] = materialSurface;
  return materialMap;
}

Acts::MaterialMapJsonConverter::DetectorMaterialMaps
Acts::MaterialMapJsonConverter::jsonToMaterialMaps(
    const nlohmann::json& materialmap) {
  nlohmann::json materialVolume = materialmap["Volumes"];
  GeometryHierarchyMap<const IVolumeMaterial*> hierarchyVolumeMap =
      m_volumeMaterialConverter.fromJson(materialVolume);
  VolumeMaterialMap volumeMap;
  for (std::size_t i = 0; i < hierarchyVolumeMap.size(); i++) {
    std::shared_ptr<const IVolumeMaterial> volumePointer(
        hierarchyVolumeMap.valueAt(i));
    volumeMap.insert({hierarchyVolumeMap.idAt(i), std::move(volumePointer)});
  }
  nlohmann::json materialSurface = materialmap["Surfaces"];
  GeometryHierarchyMap<const ISurfaceMaterial*> hierarchySurfaceMap =
      m_surfaceMaterialConverter.fromJson(materialSurface);
  SurfaceMaterialMap surfaceMap;
  for (std::size_t i = 0; i < hierarchySurfaceMap.size(); i++) {
    std::shared_ptr<const ISurfaceMaterial> surfacePointer(
        hierarchySurfaceMap.valueAt(i));
    surfaceMap.insert({hierarchySurfaceMap.idAt(i), std::move(surfacePointer)});
  }

  Acts::MaterialMapJsonConverter::DetectorMaterialMaps maps = {surfaceMap,
                                                               volumeMap};

  // Return the filled maps
  return maps;
}

nlohmann::json Acts::MaterialMapJsonConverter::trackingGeometryToJson(
    const Acts::TrackingGeometry& tGeometry,
    const ITrackingGeometryJsonDecorator* decorator) {
  std::vector<std::pair<GeometryIdentifier, Acts::TrackingVolumeAndMaterial>>
      volumeHierarchy;
  std::vector<
      std::pair<GeometryIdentifier, Acts::SurfaceAndMaterialWithContext>>
      surfaceHierarchy;
  convertToHierarchy(volumeHierarchy, surfaceHierarchy,
                     tGeometry.highestTrackingVolume());
  GeometryHierarchyMap<Acts::TrackingVolumeAndMaterial> hierarchyVolumeMap(
      volumeHierarchy);
  nlohmann::json jsonVolumes =
      m_volumeConverter.toJson(hierarchyVolumeMap, decorator);
  GeometryHierarchyMap<Acts::SurfaceAndMaterialWithContext> hierarchySurfaceMap(
      surfaceHierarchy);
  nlohmann::json jsonSurfaces =
      m_surfaceConverter.toJson(hierarchySurfaceMap, decorator);
  nlohmann::json hierarchyMap;
  hierarchyMap["Volumes"] = jsonVolumes;
  hierarchyMap["Surfaces"] = jsonSurfaces;
  return hierarchyMap;
}

void Acts::MaterialMapJsonConverter::convertToHierarchy(
    std::vector<std::pair<GeometryIdentifier, Acts::TrackingVolumeAndMaterial>>&
        volumeHierarchy,
    std::vector<
        std::pair<GeometryIdentifier, Acts::SurfaceAndMaterialWithContext>>&
        surfaceHierarchy,
    const Acts::TrackingVolume* tVolume) {
  auto sameId = [tVolume](const auto& pair) {
    return (tVolume->geometryId() == pair.first);
  };
  if (std::ranges::any_of(volumeHierarchy, sameId)) {
    // this volume was already visited
    return;
  }
  if ((tVolume->volumeMaterial() != nullptr ||
       m_cfg.processNonMaterial == true) &&
      m_cfg.processVolumes == true) {
    volumeHierarchy.push_back(
        {tVolume->geometryId(), defaultVolumeMaterial(tVolume)});
  }
  // there are confined volumes
  if (tVolume->confinedVolumes() != nullptr) {
    // get through the volumes
    auto& volumes = tVolume->confinedVolumes()->arrayObjects();
    // loop over the volumes
    for (auto& vol : volumes) {
      // recursive call
      convertToHierarchy(volumeHierarchy, surfaceHierarchy, vol.get());
    }
  }
  // there are dense volumes
  if (m_cfg.processDenseVolumes && !tVolume->denseVolumes().empty()) {
    // loop over the volumes
    for (auto& vol : tVolume->denseVolumes()) {
      // recursive call
      convertToHierarchy(volumeHierarchy, surfaceHierarchy, vol.get());
    }
  }
  if (tVolume->confinedLayers() != nullptr) {
    // get the layers
    auto& layers = tVolume->confinedLayers()->arrayObjects();
    // loop over the layers
    for (auto& lay : layers) {
      if (m_cfg.processRepresenting == true) {
        auto& layRep = lay->surfaceRepresentation();
        if ((layRep.surfaceMaterial() != nullptr ||
             m_cfg.processNonMaterial == true) &&
            layRep.geometryId() != GeometryIdentifier()) {
          surfaceHierarchy.push_back(
              {layRep.geometryId(),
               defaultSurfaceMaterial(layRep.getSharedPtr(), m_cfg.context)});
        }
      }
      if (lay->approachDescriptor() != nullptr &&
          m_cfg.processApproaches == true) {
        for (auto& asf : lay->approachDescriptor()->containedSurfaces()) {
          if (asf->surfaceMaterial() != nullptr ||
              m_cfg.processNonMaterial == true) {
            surfaceHierarchy.push_back(
                {asf->geometryId(),
                 defaultSurfaceMaterial(asf->getSharedPtr(), m_cfg.context)});
          }
        }
      }
      if (lay->surfaceArray() != nullptr && m_cfg.processSensitives == true) {
        for (auto& ssf : lay->surfaceArray()->surfaces()) {
          if (ssf->surfaceMaterial() != nullptr ||
              m_cfg.processNonMaterial == true) {
            auto sp = ssf->getSharedPtr();
            auto sm = defaultSurfaceMaterial(sp, m_cfg.context);
            auto id = ssf->geometryId();

            std::pair p{id, sm};
            surfaceHierarchy.push_back(p);
          }
        }
      }
    }
  }
  // Let's finally check the boundaries
  for (auto& bsurf : tVolume->boundarySurfaces()) {
    // the surface representation
    auto& bssfRep = bsurf->surfaceRepresentation();
    if (bssfRep.geometryId().volume() == tVolume->geometryId().volume() &&
        m_cfg.processBoundaries == true) {
      if (bssfRep.surfaceMaterial() != nullptr ||
          m_cfg.processNonMaterial == true) {
        surfaceHierarchy.push_back(
            {bssfRep.geometryId(),
             defaultSurfaceMaterial(bssfRep.getSharedPtr(), m_cfg.context)});
      }
    }
  }
}
