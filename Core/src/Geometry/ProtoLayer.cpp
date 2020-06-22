// This file is part of the Acts project.
//
// Copyright (C) 2018-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/ProtoLayer.hpp"
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <cmath>

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

namespace Acts {

ProtoLayer::ProtoLayer(const GeometryContext& gctx,
                       const std::vector<const Surface*>& surfaces,
                       const std::map<BinningValue, BinningRange>& binningMap)
    : binning(binningMap), m_surfaces(surfaces) {
  measure(gctx, surfaces);
}

ProtoLayer::ProtoLayer(
    const GeometryContext& gctx,
    const std::vector<std::shared_ptr<const Surface>>& surfaces,
    const std::map<BinningValue, BinningRange>& binningMap)
    : binning(binningMap), m_surfaces(unpack_shared_vector(surfaces)) {
  measure(gctx, m_surfaces);
}

double ProtoLayer::min(BinningValue bval, bool addenv) {
  if (addenv) {
    return extent.min(bval) - envelope[bval].first;
  }
  return extent.min(bval);
}

double ProtoLayer::max(BinningValue bval, bool addenv) {
  if (addenv) {
    return extent.max(bval) + envelope[bval].second;
  }
  return extent.max(bval);
}

double ProtoLayer::medium(BinningValue bval, bool addenv) {
  return 0.5 * (min(bval, addenv) + max(bval, addenv));
}

double ProtoLayer::range(BinningValue bval, bool addenv) {
  return std::abs(max(bval, addenv) - min(bval, addenv));
}

std::ostream& ProtoLayer::toStream(std::ostream& sl) const {
  sl << "ProtoLayer with dimensions (min/max)" << std::endl;
  extent.toStream(sl);
  return sl;
}

void ProtoLayer::measure(const GeometryContext& gctx,
                         const std::vector<const Surface*>& surfaces) {
  for (const auto& sf : surfaces) {
    auto sfPolyhedron = sf->polyhedronRepresentation(gctx, 1);
    const DetectorElementBase* element = sf->associatedDetectorElement();
    if (element != nullptr) {
      // Take the thickness in account if necessary
      double thickness = element->thickness();
      // We need a translation along and opposite half thickness
      Vector3D sfNormal = sf->normal(gctx, sf->center(gctx));
      std::vector<double> deltaT = {-0.5 * thickness, 0.5 * thickness};
      for (const auto& dT : deltaT) {
        Transform3D dtransform = Transform3D::Identity();
        dtransform.pretranslate(dT * sfNormal);
        extent.extend(sfPolyhedron.extent(dtransform));
      }
      continue;
    }
    extent.extend(sfPolyhedron.extent());
  }
}

void ProtoLayer::add(const GeometryContext& gctx, const Surface& surface) {
  m_surfaces.push_back(&surface);
  measure(gctx, m_surfaces);
}

}  // namespace Acts