// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Plugins/Json/MaterialJsonConverter.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConeBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"

#include <algorithm>

void Acts::to_json(nlohmann::json& j,
                   const Acts::SurfaceAndMaterialWithContext& surface) {
  toJson(j, std::get<0>(surface), std::get<2>(surface));
  to_json(j, std::get<1>(surface).get());
}

void Acts::to_json(nlohmann::json& j, const Acts::Surface& surface) {
  Acts::GeometryContext gctx;
  j = SurfaceJsonConverter::toJson(gctx, surface);
}

void Acts::to_json(nlohmann::json& j,
                   const std::shared_ptr<const Acts::Surface>& surface) {
  Acts::GeometryContext gctx;
  j = SurfaceJsonConverter::toJson(gctx, *(surface.get()));
}

void Acts::toJson(nlohmann::json& j,
                  const std::shared_ptr<const Acts::Surface>& surface,
                  const Acts::GeometryContext& gctx) {
  j = SurfaceJsonConverter::toJson(gctx, *(surface.get()));
}

std::shared_ptr<Acts::Surface> Acts::SurfaceJsonConverter::fromJson(
    const nlohmann::json& j) {
  // The types to understand the types
  auto sType = j["type"].get<Surface::SurfaceType>();
  auto bType = j["bounds"]["type"].get<SurfaceBounds::BoundsType>();

  std::shared_ptr<Surface> mutableSf = nullptr;

  /// Unroll the types
  switch (sType) {
    // Surface is a plane surface
    case Surface::SurfaceType::Plane:
      switch (bType) {
        case SurfaceBounds::BoundsType::eEllipse:
          mutableSf = surfaceFromJsonT<PlaneSurface, EllipseBounds>(j);
          break;
        case SurfaceBounds::BoundsType::eRectangle:
          mutableSf = surfaceFromJsonT<PlaneSurface, RectangleBounds>(j);
          break;
        case SurfaceBounds::BoundsType::eTrapezoid:
          mutableSf = surfaceFromJsonT<PlaneSurface, TrapezoidBounds>(j);
          break;

        case SurfaceBounds::BoundsType::eBoundless:
          mutableSf = surfaceFromJsonT<PlaneSurface, void>(j);
          break;
        default:
          throw std::invalid_argument("Invalid bounds type " +
                                      std::to_string(bType) +
                                      " for plane surface");
      }
      break;
    // Surface is a disc surface
    case Surface::SurfaceType::Disc:
      switch (bType) {
        case SurfaceBounds::BoundsType::eAnnulus:
          mutableSf = surfaceFromJsonT<DiscSurface, AnnulusBounds>(j);
          break;
        case SurfaceBounds::BoundsType::eDisc:
          mutableSf = surfaceFromJsonT<DiscSurface, RadialBounds>(j);
          break;
        case SurfaceBounds::BoundsType::eDiscTrapezoid:
          mutableSf = surfaceFromJsonT<DiscSurface, DiscTrapezoidBounds>(j);
          break;
        default:
          throw std::invalid_argument("Invalid bounds type " +
                                      std::to_string(bType) +
                                      " for disc surface");
      }
      break;
    // Surface is a cylinder surface
    case Surface::SurfaceType::Cylinder:
      mutableSf = surfaceFromJsonT<CylinderSurface, CylinderBounds>(j);
      break;
    // Surface is a cone surface
    case Surface::SurfaceType::Cone:
      mutableSf = surfaceFromJsonT<ConeSurface, ConeBounds>(j);
      break;
    // Surface is a straw surface
    case Surface::SurfaceType::Straw:
      mutableSf = surfaceFromJsonT<StrawSurface, LineBounds>(j);
      break;
    // Surface is a perigee surface
    case Surface::SurfaceType::Perigee:
      mutableSf = Surface::makeShared<PerigeeSurface>(
          Transform3JsonConverter::fromJson(j["transform"]));
      break;
    default:
      throw std::invalid_argument("Invalid surface type " +
                                  std::to_string(sType));
  }

  throw_assert(mutableSf, "Could not create surface from json");

  GeometryIdentifier geoID(j["geo_id"]);
  mutableSf->assignGeometryId(geoID);
  // Add material
  if (j.find("material") != j.end() && !j["material"].empty()) {
    const ISurfaceMaterial* surfaceMaterial = nullptr;
    from_json(j, surfaceMaterial);
    std::shared_ptr<const ISurfaceMaterial> sharedSurfaceMaterial(
        surfaceMaterial);
    mutableSf->assignSurfaceMaterial(sharedSurfaceMaterial);
  }

  return mutableSf;
}

nlohmann::json Acts::SurfaceJsonConverter::toJson(const GeometryContext& gctx,
                                                  const Surface& surface,
                                                  const Options& options) {
  nlohmann::json jSurface;
  const auto& sBounds = surface.bounds();
  const auto sTransform = surface.transform(gctx);

  jSurface["transform"] =
      Transform3JsonConverter::toJson(sTransform, options.transformOptions);
  jSurface["type"] = surface.type();
  // Transform is always needed
  jSurface["bounds"] = SurfaceBoundsJsonConverter::toJson(sBounds);
  jSurface["geo_id"] = surface.geometryId().value();
  if (surface.surfaceMaterial() != nullptr && options.writeMaterial) {
    jSurface["material"] = nlohmann::json(surface.surfaceMaterial());
  }
  return jSurface;
}

nlohmann::json Acts::SurfaceJsonConverter::toJsonDetray(
    const GeometryContext& gctx, const Surface& surface,
    const Options& options) {
  nlohmann::json jSurface;
  const auto& sBounds = surface.bounds();
  const auto sTransform = surface.transform(gctx);

  jSurface["transform"] =
      Transform3JsonConverter::toJson(sTransform, options.transformOptions);

  auto jMask =
      SurfaceBoundsJsonConverter::toJsonDetray(sBounds, options.portal);
  jSurface["mask"] = jMask;
  jSurface["source"] = surface.geometryId().value();
  jSurface["barcode"] = 0;
  jSurface["type"] =
      options.portal ? 0 : (surface.geometryId().sensitive() > 0 ? 1u : 2u);

  return jSurface;
}
