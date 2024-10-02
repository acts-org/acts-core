// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/DetectorJsonConverter.hpp"

#include "Acts/Detector/Detector.hpp"
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Plugins/Json/DetectorVolumeFinderJsonConverter.hpp"
#include "Acts/Plugins/Json/DetectorVolumeJsonConverter.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Plugins/Json/IndexedSurfacesJsonConverter.hpp"
#include "Acts/Plugins/Json/MaterialJsonConverter.hpp"
#include "Acts/Plugins/Json/PortalJsonConverter.hpp"
#include "Acts/Plugins/Json/VolumeBoundsJsonConverter.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <ctime>
#include <map>
#include <memory>
#include <set>
#include <string>

nlohmann::json Acts::DetectorJsonConverter::toJson(
    const GeometryContext& gctx, const Experimental::Detector& detector,
    const Options& options) {
  // Get the time stamp
  std::time_t tt = 0;
  std::time(&tt);
  auto ti = std::localtime(&tt);

  nlohmann::json jDetector;

  std::size_t nSurfaces = 0;
  std::vector<const Experimental::Portal*> portals;

  for (const auto* volume : detector.volumes()) {
    nSurfaces += volume->surfaces().size();
    for (const auto& portal : volume->portals()) {
      if (!rangeContainsValue(portals, portal)) {
        portals.push_back(portal);
      }
    }
  }

  // Write the actual data objects
  nlohmann::json jData;
  jData["name"] = detector.name();

  // The portals are written first
  auto volumes = detector.volumes();
  nlohmann::json jPortals;

  for (const auto& portal : portals) {
    auto jPortal = PortalJsonConverter::toJson(
        gctx, *portal, volumes, options.volumeOptions.portalOptions);
    jPortals.push_back(jPortal);
  }
  jData["portals"] = jPortals;

  // The volumes are written next, with portals already defined, the volumes
  // will only index those
  nlohmann::json jVolumes;
  for (const auto& volume : volumes) {
    auto jVolume = DetectorVolumeJsonConverter::toJson(
        gctx, *volume, volumes, portals, options.volumeOptions);
    jVolumes.push_back(jVolume);
  }
  jData["volumes"] = jVolumes;
  jData["volume_finder"] = DetectorVolumeFinderJsonConverter::toJson(
      detector.detectorVolumeFinder());

  // Write the header
  nlohmann::json jHeader;
  jHeader["detector"] = detector.name();
  jHeader["type"] = "acts";
  char buffer[100];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ti);
  jHeader["date"] = std::string(buffer);
  jHeader["surface_count"] = nSurfaces;
  jHeader["portal_count"] = portals.size();
  jHeader["volume_count"] = detector.volumes().size();
  jDetector["header"] = jHeader;
  jDetector["data"] = jData;
  return jDetector;
}

nlohmann::json Acts::DetectorJsonConverter::toJsonDetray(
    const GeometryContext& gctx, const Experimental::Detector& detector,
    const Options& options) {
  // Get the time stamp
  std::time_t tt = 0;
  std::time(&tt);
  auto ti = std::localtime(&tt);

  nlohmann::json jFile;

  nlohmann::json jCommonHeader;
  jCommonHeader["detector"] = detector.name();
  char buffer[100];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ti);
  jCommonHeader["date"] = std::string(buffer);
  jCommonHeader["version"] = "detray - 0.44.0";
  jCommonHeader["tag"] = "geometry";

  // Three sub sections are created
  // (1) Geometry
  nlohmann::json jGeometry;
  nlohmann::json jGeometryData;
  nlohmann::json jGeometryHeader;
  nlohmann::json jGeometryVolumes;
  // (2) Surface grids
  nlohmann::json jSurfaceGrids;
  nlohmann::json jSurfaceGridsData;
  nlohmann::json jSurfaceGridsInfoCollection;
  nlohmann::json jSurfaceGridsHeader;
  // (3) Material
  nlohmann::json jMaterial;
  nlohmann::json jMaterialData;
  nlohmann::json jMaterialHeader;
  nlohmann::json jMaterialGrids;

  // Counters:
  std::size_t nSurfaces = 0;
  std::size_t nGrids = 0;

  // Main loop over volumes
  for (auto [iv, volume] : enumerate(detector.volumes())) {
    // Write the volume information
    nlohmann::json jVolume;
    jVolume["name"] = volume->name();

    // Write the transform and bounds information
    jVolume["transform"] = Transform3JsonConverter::toJson(
        volume->transform(gctx), options.volumeOptions.transformOptions);
    jVolume["bounds"] =
        VolumeBoundsJsonConverter::toJson(volume->volumeBounds());
    auto volumeBoundsType = volume->volumeBounds().type();
    if (volumeBoundsType == VolumeBounds::BoundsType::eCylinder) {
      jVolume["type"] = 0u;
    } else if (volumeBoundsType == VolumeBounds::BoundsType::eCuboid) {
      jVolume["type"] = 4u;
    } else {
      throw std::runtime_error("Unsupported volume bounds type");
    }
    // Link to itself
    jVolume["index"] = iv;

    // Acceleration link if there
    nlohmann::json jSurfacesDelegate = IndexedSurfacesJsonConverter::toJson(
        volume->internalNavigation(), true);
    if (!jSurfacesDelegate.is_null()) {
      // Colplete the grid json for detray usage
      jSurfacesDelegate["owner_link"] = iv;
      // jSurfacesDelegate["acc_link"] =
      nlohmann::json jSurfaceGridsCollection;
      jSurfaceGridsCollection.push_back(jSurfacesDelegate);

      nlohmann::json jSurfaceGridsInfo;
      jSurfaceGridsInfo["volume_link"] = iv;
      jSurfaceGridsInfo["grid_data"] = jSurfaceGridsCollection;
      jSurfaceGridsInfoCollection.push_back(jSurfaceGridsInfo);
    }

    // Grids per volume
    nlohmann::json jMaterialVolumeGrids;
    nlohmann::json jMaterialVolumeGridsData;
    jMaterialVolumeGrids["volume_link"] = iv;
    std::map<std::size_t, std::size_t> gridLinks;

    // Write the surfaces - patch bounds & augment with self links
    std::size_t sIndex = 0;
    nlohmann::json jSurfaces;
    for (const auto& s : volume->surfaces()) {
      auto jSurface = SurfaceJsonConverter::toJsonDetray(
          gctx, *s, options.volumeOptions.surfaceOptions);
      DetrayJsonHelper::addVolumeLink(jSurface["mask"], iv);
      jSurface["index_in_coll"] = sIndex;
      jSurfaces.push_back(jSurface);

      // Check for material
      if (s->surfaceMaterial() != nullptr) {
        nlohmann::json jSurfaceMaterial = MaterialJsonConverter::toJsonDetray(
            *s->surfaceMaterial(), *s, sIndex, gridLinks);
        if (!jSurfaceMaterial.empty()) {
          ++nGrids;
          jMaterialVolumeGridsData.push_back(jSurfaceMaterial);
        }
      }
      ++sIndex;
    }

    // Write the portals - we need oriented surfaces for this
    auto orientedSurfaces =
        volume->volumeBounds().orientedSurfaces(volume->transform(gctx));
    // Write the portals - they will end up in the surface container
    for (const auto& [ip, p] : enumerate(volume->portals())) {
      auto [jPortalSurfaces, portalSubSplits] = (toJsonDetray(
          gctx, *p, ip, *volume, orientedSurfaces, detector.volumes(),
          options.volumeOptions.portalOptions));
      std::size_t splitSurfaceIdx = 0;
      for (auto& jSurface : jPortalSurfaces) {
        jSurface["index_in_coll"] = sIndex;
        jSurfaces.push_back(jSurface);
        const Surface* pSurface =
            portalSubSplits.empty() ? (&p->surface())
                                    : portalSubSplits[splitSurfaceIdx++].get();
        // Check for material
        if (p->surface().surfaceMaterial() != nullptr) {
          nlohmann::json jSurfaceMaterial = MaterialJsonConverter::toJsonDetray(
              *p->surface().surfaceMaterial(), *pSurface, sIndex, gridLinks);
          if (!jSurfaceMaterial.empty()) {
            ++nGrids;
            jMaterialVolumeGridsData.push_back(jSurfaceMaterial);
          }
        }
        ++sIndex;
      }
    }
    // If material was found, keep it
    if (!jMaterialVolumeGridsData.empty()) {
      jMaterialVolumeGrids["grid_data"] = {jMaterialVolumeGridsData};
      jMaterialGrids.push_back(jMaterialVolumeGrids);
    }

    // Surfaces go into the volume
    jVolume["surfaces"] = jSurfaces;
    nSurfaces += jSurfaces.size();
    jGeometryVolumes.push_back(jVolume);
  }

  jGeometryData["volumes"] = jGeometryVolumes;
  jGeometryData["volume_grid"] = DetectorVolumeFinderJsonConverter::toJson(
      detector.detectorVolumeFinder(), true);

  // Collect it (1) - Geometry
  jGeometryHeader["type"] = "detray";
  jGeometryHeader["common"] = jCommonHeader;
  jGeometryHeader["surface_count"] = nSurfaces;
  jGeometryHeader["volume_count"] = detector.volumes().size();
  jGeometry["header"] = jGeometryHeader;
  jGeometry["data"] = jGeometryData;
  jFile["geometry"] = jGeometry;

  // Collect it (2) - Grid
  jCommonHeader["tag"] = "surface_grids";
  jSurfaceGridsHeader["common"] = jCommonHeader;
  jSurfaceGridsData["grids"] = jSurfaceGridsInfoCollection;
  jSurfaceGridsHeader["grid_count"] = jSurfaceGridsInfoCollection.size();
  jSurfaceGrids["header"] = jSurfaceGridsHeader;
  jSurfaceGrids["data"] = jSurfaceGridsData;
  jFile["surface_grids"] = jSurfaceGrids;

  // Collect it (3) - Material
  jCommonHeader["tag"] = "material_maps";
  jMaterialHeader["common"] = jCommonHeader;
  jMaterialData["grids"] = jMaterialGrids;
  jMaterialHeader["grid_count"] = nGrids;
  jMaterial["header"] = jMaterialHeader;
  jMaterial["data"] = jMaterialData;
  jFile["material"] = jMaterial;

  return jFile;
}

std::shared_ptr<Acts::Experimental::Detector>
Acts::DetectorJsonConverter::fromJson(const GeometryContext& gctx,
                                      const nlohmann::json& jDetector) {
  // Read back all the data
  auto jData = jDetector["data"];
  auto jVolumes = jData["volumes"];
  auto jPortals = jData["portals"];
  const std::string name = jData["name"];

  std::vector<std::shared_ptr<Experimental::DetectorVolume>> volumes;
  std::vector<std::shared_ptr<Experimental::Portal>> portals;

  for (const auto& jVolume : jVolumes) {
    auto volume = DetectorVolumeJsonConverter::fromJson(gctx, jVolume);
    volumes.push_back(volume);
  }

  for (const auto& jPortal : jPortals) {
    auto portal = PortalJsonConverter::fromJson(gctx, jPortal, volumes);
    portals.push_back(portal);
  }

  // Patch all the portals of the volumes
  for (auto [iv, v] : enumerate(volumes)) {
    // Get the Volumes and update the portals with the loaded ones
    auto jVolume = jVolumes[iv];
    std::vector<std::size_t> portalLinks = jVolume["portal_links"];
    for (auto [ip, ipl] : enumerate(portalLinks)) {
      auto portal = portals[ipl];
      v->updatePortal(portal, ip);
    }
  }
  return Experimental::Detector::makeShared(name, volumes,
                                            Experimental::tryRootVolumes());
}
