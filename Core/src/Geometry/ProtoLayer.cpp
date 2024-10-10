// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/ProtoLayer.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Utilities/Helpers.hpp"

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

namespace Acts {

ProtoLayer::ProtoLayer(const GeometryContext& gctx,
                       const std::vector<const Surface*>& surfaces,
                       const Transform3& transformIn)
    : transform(transformIn), m_surfaces(surfaces) {
  measure(gctx, surfaces);
}

ProtoLayer::ProtoLayer(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<const Surface>>& surfaces,
    const Transform3& transformIn)
    : transform(transformIn), m_surfaces(unpack_shared_vector(surfaces)) {
  measure(gctx, m_surfaces);
}

ProtoLayer::ProtoLayer(const GeometryContext& gctx,
                       const std::vector<std::shared_ptr<Surface>>& surfaces,
                       const Transform3& transformIn)
    : transform(transformIn) {
  m_surfaces.reserve(surfaces.size());
  for (const auto& sf : surfaces) {
    m_surfaces.push_back(sf.get());
  }
  measure(gctx, m_surfaces);
}

double ProtoLayer::min(BinningValue bval, bool addenv) const {
  if (addenv) {
    return extent.min(bval) - envelope[bval][0u];
  }
  return extent.min(bval);
}

double ProtoLayer::max(BinningValue bval, bool addenv) const {
  if (addenv) {
    return extent.max(bval) + envelope[bval][1u];
  }
  return extent.max(bval);
}

double ProtoLayer::medium(BinningValue bval, bool addenv) const {
  return 0.5 * (min(bval, addenv) + max(bval, addenv));
}

double ProtoLayer::range(BinningValue bval, bool addenv) const {
  return std::abs(max(bval, addenv) - min(bval, addenv));
}

std::ostream& ProtoLayer::toStream(std::ostream& sl) const {
  sl << "ProtoLayer with dimensions (min/max)" << std::endl;
  sl << extent.toString();
  return sl;
}

void ProtoLayer::measure(const GeometryContext& gctx,
                         const std::vector<const Surface*>& surfaces) {
  for (const auto& sf : surfaces) {
    // To prevent problematic isInsidePolygon check for straw surfaces with only
    // one lseg
    int lseg = (sf->type() != Surface::Straw) ? 1 : 2;
    auto sfPolyhedron = sf->polyhedronRepresentation(gctx, lseg);
    const DetectorElementBase* element = sf->associatedDetectorElement();
    const auto* regSurface = dynamic_cast<const RegularSurface*>(sf);
    if (element != nullptr && regSurface != nullptr) {
      // Take the thickness in account if necessary
      double thickness = element->thickness();
      // We need a translation along and opposite half thickness
      Vector3 sfNormal = regSurface->normal(gctx, sf->center(gctx));
      for (const auto& dT : {-0.5 * thickness, 0.5 * thickness}) {
        Transform3 dtransform = transform * Translation3{dT * sfNormal};
        extent.extend(sfPolyhedron.extent(dtransform));
      }
      continue;
    }
    extent.extend(sfPolyhedron.extent(transform));
  }
}

void ProtoLayer::add(const GeometryContext& gctx, const Surface& surface) {
  m_surfaces.push_back(&surface);
  measure(gctx, m_surfaces);
}

}  // namespace Acts
