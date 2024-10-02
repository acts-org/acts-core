// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/PlaneLayer.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GenericApproachDescriptor.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <algorithm>
#include <vector>

Acts::PlaneLayer::PlaneLayer(const Transform3& transform,
                             std::shared_ptr<const PlanarBounds>& pbounds,
                             std::unique_ptr<SurfaceArray> surfaceArray,
                             double thickness,
                             std::unique_ptr<ApproachDescriptor> ades,
                             LayerType laytyp)
    : PlaneSurface(transform, pbounds),
      Layer(std::move(surfaceArray), thickness, std::move(ades), laytyp) {
  // @todo create representing volume
  // register the layer to the surface
  Acts::PlaneSurface::associateLayer(*this);
  // deal with the approach descriptor
  if (!m_approachDescriptor && m_surfaceArray) {
    buildApproachDescriptor();
  }
  // register the layer to the approach descriptor
  if (m_approachDescriptor) {
    approachDescriptor()->registerLayer(*this);
  }
}

const Acts::PlaneSurface& Acts::PlaneLayer::surfaceRepresentation() const {
  return (*this);
}

Acts::PlaneSurface& Acts::PlaneLayer::surfaceRepresentation() {
  return (*this);
}

void Acts::PlaneLayer::buildApproachDescriptor() {
  // delete it
  m_approachDescriptor.reset(nullptr);
  // delete the surfaces
  std::vector<std::shared_ptr<const Acts::Surface>> aSurfaces;
  // get the appropriate transform, the center and the normal vector

  //@todo fix with representing volume
  const Transform3& lTransform = transform(GeometryContext());
  RotationMatrix3 lRotation = lTransform.rotation();
  const Vector3& lCenter = center(GeometryContext());
  const Vector3& lVector = normal(GeometryContext(), lCenter);
  // create new surfaces
  const Transform3 apnTransform = Transform3(
      Translation3(lCenter - 0.5 * Layer::m_layerThickness * lVector) *
      lRotation);
  const Transform3 appTransform = Transform3(
      Translation3(lCenter + 0.5 * Layer::m_layerThickness * lVector) *
      lRotation);
  // create the new surfaces
  aSurfaces.push_back(Surface::makeShared<Acts::PlaneSurface>(
      apnTransform, PlaneSurface::m_bounds));
  aSurfaces.push_back(Surface::makeShared<Acts::PlaneSurface>(
      appTransform, PlaneSurface::m_bounds));
  // set the layer and make TrackingGeometry
  for (auto& sfPtr : aSurfaces) {
    auto mutableSf = const_cast<Surface*>(sfPtr.get());
    mutableSf->associateLayer(*this);
  }
  // @todo check if we can provide the layer at surface creation
  m_approachDescriptor =
      std::make_unique<const GenericApproachDescriptor>(std::move(aSurfaces));
}
