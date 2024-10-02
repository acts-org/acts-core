// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/Json/ActsJson.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

// Custom Json encoder/decoders.
namespace Acts {
class SurfaceBounds;

void to_json(nlohmann::json& j, const SurfaceBounds& bounds);

namespace SurfaceBoundsJsonConverter {

/// Interface with detray conversion option
/// @param bounds is the bounds object
/// @return the json object
nlohmann::json toJson(const SurfaceBounds& bounds);

/// Interface with detray conversion option
/// @param bounds is the bounds object
/// @param portal is the flag for conversion into detray portal format
///
/// @note reading back detray json format to Acts is not supported
///
/// @return the json object
nlohmann::json toJsonDetray(const SurfaceBounds& bounds, bool portal = false);

/// Conversion to surfaceBounds from json
///
/// The type is given as a template argument in order to be able
/// to construct the correct fitting types for surfaces.
///
/// @param j the read-in json object
///
/// @return a shared_ptr to a surface object for type polymorphism
template <typename bounds_t>
std::shared_ptr<const bounds_t> fromJson(const nlohmann::json& j) {
  const std::size_t kValues = bounds_t::BoundValues::eSize;
  std::array<ActsScalar, kValues> bValues{};
  std::vector<ActsScalar> bVector = j["values"];
  std::copy_n(bVector.begin(), kValues, bValues.begin());
  return std::make_shared<const bounds_t>(bValues);
}

}  // namespace SurfaceBoundsJsonConverter

// This macro create a conversion for the surface bounds type
NLOHMANN_JSON_SERIALIZE_ENUM(
    SurfaceBounds::BoundsType,
    {{SurfaceBounds::BoundsType::eCone, "ConeBounds"},
     {SurfaceBounds::BoundsType::eCylinder, "CylinderBounds"},
     {SurfaceBounds::BoundsType::eDiamond, "DiamondBounds"},
     {SurfaceBounds::BoundsType::eDisc, "RadialBounds"},
     {SurfaceBounds::BoundsType::eEllipse, "EllipseBounds"},
     {SurfaceBounds::BoundsType::eLine, "LineBounds"},
     {SurfaceBounds::BoundsType::eRectangle, "RectangleBounds"},
     {SurfaceBounds::BoundsType::eTrapezoid, "TrapezoidBounds"},
     {SurfaceBounds::BoundsType::eTriangle, "TriangleBounds"},
     {SurfaceBounds::BoundsType::eDiscTrapezoid, "DiscTrapezoidBounds"},
     {SurfaceBounds::BoundsType::eConvexPolygon, "ConvexPolygonBounds"},
     {SurfaceBounds::BoundsType::eAnnulus, "AnnulusBounds"},
     {SurfaceBounds::BoundsType::eBoundless, "Boundless"},
     {SurfaceBounds::BoundsType::eOther, "OtherBounds"}})

}  // namespace Acts
