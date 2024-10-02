// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/PortalSvgConverter.hpp"

#include "Acts/Detector/Portal.hpp"
#include "Acts/Navigation/PortalNavigation.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"

namespace {

/// Helper method to make a proto link implementation
///
/// @param portalOptions are the options containing portal draw length and volume colour map
/// @param position is the start position of the link
/// @param direction is the direction of the link
/// @param dVolume the volume this link is directing to
///
/// @return a protoLink
Acts::Svg::ProtoLink makeProtoLink(
    const Acts::Svg::PortalConverter::Options& portalOptions,
    const Acts::Vector3& position, const Acts::Vector3& direction,
    const Acts::Experimental::DetectorVolume* dVolume) {
  Acts::Svg::ProtoLink pLink;
  Acts::Vector3 end3 = position + portalOptions.linkLength * direction;
  pLink._start = {position.x(), position.y(), position.z()};
  pLink._end = {end3.x(), end3.y(), end3.z()};
  auto linkIndexCandidate = portalOptions.volumeIndices.find(dVolume);
  if (linkIndexCandidate != portalOptions.volumeIndices.end()) {
    pLink._link_index = linkIndexCandidate->second;
  }
  return pLink;
}

/// Helper method to convert a multi link
///
/// @param gctx is the geometry context
/// @param multiLink is the mulit link to be converted
/// @param surface is the portal surface
/// @param refPosition is the reference position which might be needed to decode
/// @param portalOptions are the options containing portal draw length and volume colour map
/// @param sign with respect to the normal vector
///
/// @return it will return the proto links
std::vector<Acts::Svg::ProtoLink> convertMultiLink(
    const Acts::GeometryContext& gctx,
    const Acts::Experimental::BoundVolumesGrid1Navigation& multiLink,
    const Acts::Surface& surface, const Acts::Vector3& refPosition,
    const Acts::Svg::PortalConverter::Options& portalOptions,
    int sign) noexcept(false) {
  const auto* regSurface = dynamic_cast<const Acts::RegularSurface*>(&surface);
  if (regSurface == nullptr) {
    throw std::invalid_argument(
        "convertMultiLink: surface is not RegularSurface.");
  }
  // The return links
  std::vector<Acts::Svg::ProtoLink> pLinks;
  const auto& volumes = multiLink.indexedUpdater.extractor.dVolumes;
  const auto& casts = multiLink.indexedUpdater.casts;

  // Generate the proto-links of the multi-link
  for (auto [il, v] : Acts::enumerate(volumes)) {
    Acts::Vector3 position = refPosition;
    if constexpr (decltype(multiLink.indexedUpdater)::grid_type::DIM == 1u) {
      // Get the binning value
      Acts::BinningValue bValue = casts[0u];
      // Get the boundaries - take care, they are in local coordinates
      const auto& boundaries =
          multiLink.indexedUpdater.grid.axes()[0u]->getBinEdges();

      Acts::ActsScalar refC = 0.5 * (boundaries[il + 1u] + boundaries[il]);

      if (bValue == Acts::BinningValue::binR) {
        Acts::ActsScalar phi = Acts::VectorHelpers::phi(refPosition);
        position = Acts::Vector3(refC * std::cos(phi), refC * std::sin(phi),
                                 refPosition.z());
      } else if (bValue == Acts::BinningValue::binZ) {
        // correct to global
        refC += surface.transform(gctx).translation().z();
        position[2] = refC;
      } else if (bValue == Acts::BinningValue::binPhi) {
        Acts::ActsScalar r = Acts::VectorHelpers::perp(refPosition);
        position = Acts::Vector3(r * std::cos(refC), r * std::sin(refC),
                                 refPosition.z());
      } else {
        throw std::invalid_argument("convertMultiLink: incorrect binning.");
      }
      Acts::Vector3 direction = regSurface->normal(gctx, position);
      pLinks.push_back(makeProtoLink(portalOptions, position,
                                     Acts::Vector3(sign * direction), v));
    }
  }
  return pLinks;
}

}  // namespace

Acts::Svg::ProtoPortal Acts::Svg::PortalConverter::convert(
    const GeometryContext& gctx, const Experimental::Portal& portal,
    const PortalConverter::Options& portalOptions) {
  ProtoPortal pPortal;
  // First convert the surface
  pPortal._surface = SurfaceConverter::convert(gctx, portal.surface(),
                                               portalOptions.surfaceOptions);

  // Reference point and direction
  Vector3 rPos(0., 0., 0);
  Vector3 rDir(0., 0., 1);
  const auto& surface = portal.surface();
  const auto surfaceTransform = portal.surface().transform(gctx);
  const auto surfaceTranslation = surfaceTransform.translation().eval();
  const auto surfaceType = surface.bounds().type();
  const auto& boundValues = surface.bounds().values();
  switch (surfaceType) {
    case SurfaceBounds::eCylinder: {
      // Get phi
      ActsScalar r = boundValues[0u];
      ActsScalar aphi = boundValues[3u];
      rPos = Vector3(r * std::cos(aphi), r * std::sin(aphi),
                     surfaceTranslation.z());
    } break;
    case SurfaceBounds::eDisc: {
      // Get phi
      ActsScalar r = 0.5 * (boundValues[0u] + boundValues[1u]);
      ActsScalar aphi = boundValues[3u];
      rPos = Vector3(r * std::cos(aphi), r * std::sin(aphi),
                     surfaceTranslation.z());
    } break;
    default: {
      rPos = surfaceTranslation;
    } break;
  }
  rDir = surface.normal(gctx, rPos);

  // Now convert the link objects
  const auto& updators = portal.portalNavigation();

  int sign = -1;
  for (const auto& dvu : updators) {
    // Get the instance and start the casting
    const auto* instance = dvu.instance();
    auto singleLink =
        dynamic_cast<const Experimental::SingleDetectorVolumeNavigation*>(
            instance);
    if (singleLink != nullptr) {
      pPortal._volume_links.push_back(makeProtoLink(
          portalOptions, rPos, Vector3(sign * rDir), singleLink->object()));
    }
    auto multiLink =
        dynamic_cast<const Experimental::BoundVolumesGrid1Navigation*>(
            instance);
    if (multiLink != nullptr) {
      auto pLinks = convertMultiLink(gctx, *multiLink, surface, rPos,
                                     portalOptions, sign);

      pPortal._volume_links.insert(pPortal._volume_links.end(), pLinks.begin(),
                                   pLinks.end());
    }
    // Switch to the other side
    sign += 2;
  }

  // Return the proto Portal
  return pPortal;
}
