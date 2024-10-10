// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/DetectorVolumeJsonConverter.hpp"

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Navigation/DetectorVolumeFinders.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Plugins/Json/IndexedSurfacesJsonConverter.hpp"
#include "Acts/Plugins/Json/PortalJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"
#include "Acts/Plugins/Json/VolumeBoundsJsonConverter.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <algorithm>
#include <ctime>

nlohmann::json Acts::DetectorVolumeJsonConverter::toJson(
    const GeometryContext& gctx, const Experimental::DetectorVolume& volume,
    const std::vector<const Experimental::DetectorVolume*>& detectorVolumes,
    const std::vector<const Experimental::Portal*>& portals,
    const Options& options) {
  nlohmann::json jVolume;
  jVolume["name"] = volume.name();
  jVolume["geometryId"] = volume.geometryId().volume();
  jVolume["transform"] = Transform3JsonConverter::toJson(
      volume.transform(gctx), options.transformOptions);
  jVolume["bounds"] = VolumeBoundsJsonConverter::toJson(volume.volumeBounds());
  // Write the surfaces
  nlohmann::json jSurfaces;
  std::for_each(
      volume.surfaces().begin(), volume.surfaces().end(), [&](const auto& s) {
        jSurfaces.push_back(
            SurfaceJsonConverter::toJson(gctx, *s, options.surfaceOptions));
      });
  jVolume["surfaces"] = jSurfaces;
  // And its surface navigation delegates
  nlohmann::json jSurfacesDelegate =
      IndexedSurfacesJsonConverter::toJson(volume.internalNavigation());
  jVolume["surface_navigation"] = jSurfacesDelegate;

  // Write the sub volumes
  nlohmann::json jVolumes;
  std::for_each(
      volume.volumes().begin(), volume.volumes().end(), [&](const auto& v) {
        jVolumes.push_back(toJson(gctx, *v, detectorVolumes, portals, options));
      });
  jVolume["volumes"] = jVolumes;

  // Write the portals if pre-converted as link
  nlohmann::json jPortals;
  if (!portals.empty()) {
    for (const auto* p : volume.portals()) {
      auto it = std::ranges::find(portals, p);
      if (it != portals.end()) {
        jPortals.push_back(std::distance(portals.begin(), it));
      } else {
        throw std::runtime_error("Portal not found in the list of portals");
      }
    }
    jVolume["portal_links"] = jPortals;
  } else {
    for (const auto& p : volume.portals()) {
      nlohmann::json jPortal = PortalJsonConverter::toJson(
          gctx, *p, detectorVolumes, options.portalOptions);
      jPortals.push_back(jPortal);
    }
    jVolume["portals"] = jPortals;
  }
  return jVolume;
}

std::shared_ptr<Acts::Experimental::DetectorVolume>
Acts::DetectorVolumeJsonConverter::fromJson(const GeometryContext& gctx,
                                            const nlohmann::json& jVolume) {
  std::string name = jVolume["name"];
  GeometryIdentifier geoId;
  geoId.setVolume(jVolume["geometryId"]);
  Transform3 transform =
      Transform3JsonConverter::fromJson(jVolume["transform"]);
  auto bounds = VolumeBoundsJsonConverter::fromJson(jVolume["bounds"]);

  auto jSurfaces = jVolume["surfaces"];
  auto jVolumes = jVolume["volumes"];

  // Some tooling
  auto portalGenerator = Experimental::defaultPortalGenerator();

  if (jSurfaces.empty() && jVolumes.empty()) {
    auto volume = Experimental::DetectorVolumeFactory::construct(
        portalGenerator, gctx, name, transform, std::move(bounds),
        Experimental::tryAllPortals());
    volume->assignGeometryId(geoId);
    return volume;
  }
  // Convert the surfaces
  std::vector<std::shared_ptr<Surface>> surfaces;
  for (const auto& js : jSurfaces) {
    surfaces.push_back(SurfaceJsonConverter::fromJson(js));
  }
  // Convert the volumes
  std::vector<std::shared_ptr<Experimental::DetectorVolume>> volumes;
  for (const auto& jv : jVolumes) {
    volumes.push_back(DetectorVolumeJsonConverter::fromJson(gctx, jv));
  }

  auto jSurfaceNavigation = jVolume["surface_navigation"];

  auto volume = Experimental::DetectorVolumeFactory::construct(
      portalGenerator, gctx, name, transform, std::move(bounds), surfaces,
      volumes, Experimental::tryRootVolumes(),
      IndexedSurfacesJsonConverter::fromJson(jSurfaceNavigation));
  volume->assignGeometryId(geoId);
  return volume;
}
