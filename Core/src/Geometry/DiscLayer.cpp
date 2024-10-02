// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/DiscLayer.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/CylinderVolumeBounds.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <vector>

using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

Acts::DiscLayer::DiscLayer(const Transform3& transform,
                           const std::shared_ptr<const DiscBounds>& dbounds,
                           std::unique_ptr<SurfaceArray> surfaceArray,
                           double thickness,
                           std::unique_ptr<ApproachDescriptor> ades,
                           LayerType laytyp)
    : DiscSurface(transform, dbounds),
      Layer(std::move(surfaceArray), thickness, std::move(ades), laytyp) {
  // In case we have Radial bounds
  const RadialBounds* rBounds =
      dynamic_cast<const RadialBounds*>(DiscSurface::m_bounds.get());
  if (rBounds != nullptr) {
    // The volume bounds
    auto rVolumeBounds =
        std::make_shared<CylinderVolumeBounds>(*rBounds, thickness);
    // @todo rotate around x for the avePhi if you have a sector
    m_representingVolume =
        std::make_unique<Volume>(*m_transform, rVolumeBounds);
  }
  // associate the layer to the layer surface itself
  DiscSurface::associateLayer(*this);
  // build an approach descriptor if none provided
  if (!m_approachDescriptor && m_surfaceArray) {
    buildApproachDescriptor();
  }
  // register the layer to the approach descriptor
  if (m_approachDescriptor) {
    approachDescriptor()->registerLayer(*this);
  }
}

const Acts::DiscSurface& Acts::DiscLayer::surfaceRepresentation() const {
  return (*this);
}

Acts::DiscSurface& Acts::DiscLayer::surfaceRepresentation() {
  return (*this);
}

void Acts::DiscLayer::buildApproachDescriptor() {
  // delete it
  m_approachDescriptor.reset(nullptr);
  // take the boundary surfaces of the representving volume if they exist
  if (m_representingVolume != nullptr) {
    // get the boundary surfaces
    std::vector<OrientedSurface> bSurfaces =
        m_representingVolume->volumeBounds().orientedSurfaces(
            m_representingVolume->transform());
    // fill in the surfaces into the vector
    std::vector<std::shared_ptr<const Surface>> aSurfaces;
    aSurfaces.push_back(bSurfaces.at(negativeFaceXY).surface);
    aSurfaces.push_back(bSurfaces.at(positiveFaceXY).surface);
    aSurfaces.push_back(bSurfaces.at(tubeInnerCover).surface);
    aSurfaces.push_back(bSurfaces.at(tubeOuterCover).surface);
    // create an ApproachDescriptor with Boundary surfaces
    m_approachDescriptor =
        std::make_unique<const GenericApproachDescriptor>(std::move(aSurfaces));
  }

  // @todo check if we can give the layer at curface creation
  for (auto& sfPtr : (m_approachDescriptor->containedSurfaces())) {
    if (sfPtr != nullptr) {
      auto& mutableSf = *(const_cast<Surface*>(sfPtr));
      mutableSf.associateLayer(*this);
    }
  }
}
