// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/ActSVG/SurfaceSvgConverter.hpp"

#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

Acts::Svg::ProtoSurface Acts::Svg::SurfaceConverter::convert(
    const GeometryContext& gctx, const Surface& surface,
    const SurfaceConverter::Options& cOptions) {
  ProtoSurface pSurface;

  // In case of non-template surfaces, the polyhedron does the trick
  if (!cOptions.templateSurface) {
    // Polyhedron surface for vertices needed anyway
    Polyhedron surfaceHedron =
        surface.polyhedronRepresentation(gctx, cOptions.style.quarterSegments);
    auto vertices3D = surfaceHedron.vertices;
    pSurface._vertices = vertices3D;
  } else {
    // In case it's a template surface, only the bounds matter
    // Check if planar bounds
    auto planarBounds =
        dynamic_cast<const Acts::PlanarBounds*>(&(surface.bounds()));
    if (planarBounds != nullptr) {
      auto vertices2D = planarBounds->vertices(cOptions.style.quarterSegments);
      pSurface._vertices.reserve(vertices2D.size());
      for (const auto& v2 : vertices2D) {
        pSurface._vertices.push_back({v2[0], v2[1], 0.});
      }
    } else {
      // Or annulus bounds
      auto annulusBounds =
          dynamic_cast<const Acts::AnnulusBounds*>(&(surface.bounds()));
      if (annulusBounds != nullptr) {
        auto vertices2D =
            annulusBounds->vertices(cOptions.style.quarterSegments);
        pSurface._vertices.reserve(vertices2D.size());
        for (const auto& v2 : vertices2D) {
          pSurface._vertices.push_back({v2[0], v2[1], 0.});
        }
      } else if (surface.type() == Acts::Surface::SurfaceType::Disc) {
        // Or disc bounds
        const auto& boundValues = surface.bounds().values();
        if (surface.bounds().type() == Acts::SurfaceBounds::BoundsType::eDisc) {
          // The radii
          actsvg::scalar ri = static_cast<actsvg::scalar>(boundValues[0]);
          actsvg::scalar ro = static_cast<actsvg::scalar>(boundValues[1]);
          pSurface._radii = {ri, ro};
          pSurface._opening = {
              static_cast<actsvg::scalar>(boundValues[3] - boundValues[2]),
              static_cast<actsvg::scalar>(boundValues[3] + boundValues[2])};

          actsvg::scalar pl = pSurface._opening[0];
          actsvg::scalar ph = pSurface._opening[1];

          pSurface._vertices = {
              {static_cast<actsvg::scalar>(ri * std::cos(pl)),
               static_cast<actsvg::scalar>(ri * std::sin(pl)), 0.},
              {static_cast<actsvg::scalar>(ro * std::cos(ph)),
               static_cast<actsvg::scalar>(ro * std::sin(ph)), 0.},
              {static_cast<actsvg::scalar>(ri * std::cos(pl)),
               static_cast<actsvg::scalar>(ri * std::sin(pl)), 0.},
              {static_cast<actsvg::scalar>(ro * std::cos(ph)),
               static_cast<actsvg::scalar>(ro * std::sin(ph)), 0.}};
        }
      }
    }
  }

  // Bound types and values
  const auto& boundValues = surface.bounds().values();
  auto bType = surface.bounds().type();
  auto bValues = surface.bounds().values();
  if (bType == Acts::SurfaceBounds::BoundsType::eRectangle) {
    pSurface._type = ProtoSurface::type::e_rectangle;
    // Set the measure
    pSurface._measures = {
        static_cast<actsvg::scalar>(0.5 * (boundValues[2] - boundValues[0])),
        static_cast<actsvg::scalar>(0.5 * (boundValues[3] - boundValues[1]))};
  } else if (bType == Acts::SurfaceBounds::BoundsType::eTrapezoid) {
    pSurface._type = ProtoSurface::type::e_trapez;
    // Set the measure
    pSurface._measures = {static_cast<actsvg::scalar>(boundValues[0]),
                          static_cast<actsvg::scalar>(boundValues[1]),
                          static_cast<actsvg::scalar>(boundValues[2])};
  } else if (bType == Acts::SurfaceBounds::BoundsType::eDiamond) {
    // Set the measure
    for (const auto& bv : boundValues) {
      pSurface._measures.push_back(static_cast<actsvg::scalar>(bv));
    }
  } else if (bType == Acts::SurfaceBounds::BoundsType::eAnnulus) {
    pSurface._type = ProtoSurface::type::e_trapez;
    // Set the measure
    for (const auto& bv : boundValues) {
      pSurface._measures.push_back(static_cast<actsvg::scalar>(bv));
    }
  } else if (bType == Acts::SurfaceBounds::BoundsType::eDisc) {
    pSurface._type = ProtoSurface::type::e_disc;
    // Set the openings
    actsvg::scalar ri = static_cast<actsvg::scalar>(boundValues[0]);
    actsvg::scalar ro = static_cast<actsvg::scalar>(boundValues[1]);
    actsvg::scalar zp = static_cast<actsvg::scalar>(surface.center(gctx).z());
    pSurface._radii = {ri, ro};
    pSurface._zparameters = {zp, zp};
    pSurface._opening = {
        static_cast<actsvg::scalar>(boundValues[3] - boundValues[2]),
        static_cast<actsvg::scalar>(boundValues[3] + boundValues[2])};
    // Set the measure
    for (const auto& bv : boundValues) {
      pSurface._measures.push_back(static_cast<actsvg::scalar>(bv));
    }
  }

  // Decorations
  // - Flag the material
  if (surface.surfaceMaterial() != nullptr) {
    pSurface._decorations["material"] = actsvg::svg::object{};
  }

  /// - The geometry ID as string
  actsvg::svg::object geoId{};
  geoId._id = std::to_string(surface.geometryId().value());
  pSurface._decorations["geo_id"] = geoId;

  auto [surfaceFill, surfaceStroke] = cOptions.style.fillAndStroke();
  pSurface._fill = surfaceFill;
  pSurface._stroke = surfaceStroke;

  return pSurface;
}
