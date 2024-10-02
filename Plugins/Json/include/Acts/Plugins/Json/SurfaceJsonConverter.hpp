// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/SurfaceBoundsJsonConverter.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders. Naming is mandated by nlohmann::json and thus
// can not match our naming guidelines.
namespace Acts {
class ISurfaceMaterial;

using SurfaceAndMaterialWithContext =
    std::tuple<std::shared_ptr<const Acts::Surface>,
               std::shared_ptr<const Acts::ISurfaceMaterial>,
               Acts::GeometryContext>;

/// Conversion of a pair of surface and material used for the material mapping
void to_json(nlohmann::json& j, const SurfaceAndMaterialWithContext& surface);

/// Non-contextual conversion of a surface
///
/// @note it will take the default context
void to_json(nlohmann::json& j, const Surface& surface);

/// Non-contextual conversion of a surface
///
/// @note it will take the default context
void to_json(nlohmann::json& j, const std::shared_ptr<const Surface>& surface);

/// Contextual conversion of a surface
///
/// @param j the json to be filled
/// @param surface the surface to be converted
/// @param gctx the geometry context for this
void toJson(nlohmann::json& j, const std::shared_ptr<const Surface>& surface,
            const Acts::GeometryContext& gctx);

/// Conversion to Surface from jsonn
///
/// @param j the read-in json object
///
/// @return a shared_ptr to a surface object for type polymorphism
std::shared_ptr<Surface> surfaceFromJson(const nlohmann::json& j);

/// Conversion to Surface from json in correct type
///
/// The type is given as a template argument in order to be able
/// to construct the correct fitting types for surfaces.
///
/// @param j the read-in json object
///
/// @return a shared_ptr to a typed surface object for type polymorphism
template <typename surface_t, typename bounds_t>
std::shared_ptr<surface_t> surfaceFromJsonT(const nlohmann::json& j) {
  nlohmann::json jTransform = j["transform"];
  Transform3 sTransform = Transform3JsonConverter::fromJson(jTransform);
  if constexpr (std::is_same_v<bounds_t, void>) {
    return Surface::makeShared<surface_t>(sTransform);
  } else {
    nlohmann::json jBounds = j["bounds"];
    auto sBounds = SurfaceBoundsJsonConverter::fromJson<bounds_t>(jBounds);
    return Surface::makeShared<surface_t>(sTransform, std::move(sBounds));
  }
}

namespace SurfaceJsonConverter {

struct Options {
  // The way the transforms are written out
  Transform3JsonConverter::Options transformOptions =
      Transform3JsonConverter::Options{};
  // Write the material
  bool writeMaterial = true;
  // Write surface as a portal
  bool portal = false;
};

/// Contextual conversion of a surface
///
/// @param gctx the geometry context for this
/// @param surface the surface to be converted
/// @param options the writing options for the surfaces
///
/// @return a json object representing the surface
nlohmann::json toJson(const GeometryContext& gctx, const Surface& surface,
                      const Options& options = Options{});

/// Contextual conversion of a surface - Detray export
///
/// @param gctx the geometry context for this
/// @param surface the surface to be converted
/// @param options the writing options for the surfaces
///
/// @note reading back detray json is not supported and will fail
///
/// @return a json object representing the surface
nlohmann::json toJsonDetray(const GeometryContext& gctx, const Surface& surface,
                            const Options& options = Options{});

/// @brief The Surface converter from json
///
/// @param jSurface the surface json object
///
/// @return a shared object created from json input
std::shared_ptr<Surface> fromJson(const nlohmann::json& jSurface);

}  // namespace SurfaceJsonConverter

// This macro create a conversion for the surface type
NLOHMANN_JSON_SERIALIZE_ENUM(
    Surface::SurfaceType,
    {{Surface::SurfaceType::Cone, "ConeSurface"},
     {Surface::SurfaceType::Cylinder, "CylinderSurface"},
     {Surface::SurfaceType::Disc, "DiscSurface"},
     {Surface::SurfaceType::Perigee, "PerigeeSurface"},
     {Surface::SurfaceType::Plane, "PlaneSurface"},
     {Surface::SurfaceType::Straw, "StrawSurface"},
     {Surface::SurfaceType::Curvilinear, "CurvilinearSurface"},
     {Surface::SurfaceType::Other, "Other"}})

}  // namespace Acts
