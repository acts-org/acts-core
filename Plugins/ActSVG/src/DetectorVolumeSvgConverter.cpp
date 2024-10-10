// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/DetectorVolumeSvgConverter.hpp"

#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Plugins/ActSVG/IndexedSurfacesSvgConverter.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <utility>

std::tuple<Acts::Svg::ProtoVolume, Acts::Svg::ProtoIndexedSurfaceGrid>
Acts::Svg::DetectorVolumeConverter::convert(
    const GeometryContext& gctx, const Experimental::DetectorVolume& dVolume,
    const DetectorVolumeConverter::Options& volumeOptions) {
  ProtoVolume pVolume;
  pVolume._name = dVolume.name();

  auto volumeTransform = dVolume.transform(gctx);

  // The detector volume is of cylindrical shape
  const auto& boundValues = dVolume.volumeBounds().values();
  if (dVolume.volumeBounds().type() == Acts::VolumeBounds::eCylinder) {
    // we keep 6 for the moment
    for (unsigned int ib = 0; ib < 2u; ++ib) {
      pVolume._bound_values.push_back(
          static_cast<actsvg::scalar>(boundValues[ib]));
    }
    pVolume._bound_values.push_back(
        static_cast<actsvg::scalar>(volumeTransform.translation().z()));
    for (unsigned int ib = 2u; ib < 5u; ++ib) {
      pVolume._bound_values.push_back(
          static_cast<actsvg::scalar>(boundValues[ib]));
    }
  }

  // Adding the portals to the lot
  for (auto [ip, p] : enumerate(dVolume.portals())) {
    auto pPortal =
        PortalConverter::convert(gctx, *p, volumeOptions.portalOptions);
    auto pPortalCandidate = volumeOptions.portalIndices.find(p);
    if (pPortalCandidate != volumeOptions.portalIndices.end()) {
      pPortal._name = "portal_" + std::to_string(pPortalCandidate->second);
    } else {
      pPortal._name = dVolume.name() + "_portal_" + std::to_string(ip);
    }
    pVolume._portals.push_back(pPortal);
  }

  // Adding the surfaces to the volume
  std::vector<ProtoSurface> pSurfaces;
  for (auto [is, s] : enumerate(dVolume.surfaces())) {
    auto pSurface =
        SurfaceConverter::convert(gctx, *s, volumeOptions.surfaceOptions);
    pSurface._name = dVolume.name() + "_surface_" + std::to_string(is);
    pSurfaces.push_back(pSurface);
  }
  pVolume._v_surfaces = pSurfaces;

  // Make dedicated surface grid sheets
  const auto& internalNavigationDelegate = dVolume.internalNavigation();

  IndexedSurfacesConverter::Options isOptions =
      volumeOptions.indexedSurfacesOptions;
  // Use or transfer the surface style
  if (isOptions.surfaceStyles.empty()) {
    std::pair<Acts::GeometryIdentifier, Acts::Svg::Style> style{
        dVolume.geometryId(), volumeOptions.surfaceOptions.style};
    isOptions.surfaceStyles =
        Acts::GeometryHierarchyMap<Acts::Svg::Style>({style});
  }

  auto pSurfacesGrid = IndexedSurfacesConverter::convert(
      gctx, dVolume.surfaces(), internalNavigationDelegate, isOptions);

  return {pVolume, pSurfacesGrid};
}
