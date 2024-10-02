// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/CylinderLayer.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <cstddef>
#include <vector>

namespace Acts {
class CylinderBounds;
}  // namespace Acts

using Acts::VectorHelpers::phi;

Acts::CylinderLayer::CylinderLayer(
    const Transform3& transform,
    const std::shared_ptr<const CylinderBounds>& cBounds,
    std::unique_ptr<SurfaceArray> surfaceArray, double thickness,
    std::unique_ptr<ApproachDescriptor> ades, LayerType laytyp)
    : CylinderSurface(transform, cBounds),
      Layer(std::move(surfaceArray), thickness, std::move(ades), laytyp) {
  // create the representing volume
  auto cVolumeBounds = std::make_shared<CylinderVolumeBounds>(
      *CylinderSurface::m_bounds, thickness);
  // @todo rotate around x for the avePhi if you have a sector
  m_representingVolume = std::make_unique<Volume>(*m_transform, cVolumeBounds);

  // associate the layer to the surface
  CylinderSurface::associateLayer(*this);
  // an approach descriptor is automatically created if there's a surface array
  if (!m_approachDescriptor && m_surfaceArray) {
    buildApproachDescriptor();
  }
  // register the layer to the approach descriptor surfaces
  if (m_approachDescriptor) {
    approachDescriptor()->registerLayer(*this);
  }
}

const Acts::CylinderSurface& Acts::CylinderLayer::surfaceRepresentation()
    const {
  return (*this);
}

Acts::CylinderSurface& Acts::CylinderLayer::surfaceRepresentation() {
  return (*this);
}

void Acts::CylinderLayer::buildApproachDescriptor() {
  // delete and reset as you build a new one
  m_approachDescriptor.reset(nullptr);

  // take the boundary surfaces of the representving volume if they exist
  if (m_representingVolume != nullptr) {
    // get the boundary surfaces
    std::vector<OrientedSurface> bSurfaces =
        m_representingVolume->volumeBounds().orientedSurfaces(
            m_representingVolume->transform());

    // fill in the surfaces into the vector
    std::vector<std::shared_ptr<const Surface>> aSurfaces;
    if (bSurfaces.size() > static_cast<std::size_t>(tubeInnerCover)) {
      aSurfaces.push_back(bSurfaces.at(tubeInnerCover).surface);
    }
    aSurfaces.push_back(bSurfaces.at(tubeOuterCover).surface);
    // create an ApproachDescriptor with Boundary surfaces
    m_approachDescriptor =
        std::make_unique<const GenericApproachDescriptor>(std::move(aSurfaces));
  }

  for (auto& sfPtr : (m_approachDescriptor->containedSurfaces())) {
    if (sfPtr != nullptr) {
      auto& mutableSf = *(const_cast<Surface*>(sfPtr));
      mutableSf.associateLayer(*this);
    }
  }
}
