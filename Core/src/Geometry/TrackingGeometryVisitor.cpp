// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrackingGeometryVisitor.hpp"

namespace Acts {

TrackingGeometryVisitor::~TrackingGeometryVisitor() = default;
TrackingGeometryMutableVisitor::~TrackingGeometryMutableVisitor() = default;

void TrackingGeometryVisitor::visitVolume(const TrackingVolume& /*volume*/) {
  // Default implementation is a no-op
}

void TrackingGeometryVisitor::visitPortal(const Portal& /*portal*/) {
  // Default implementation is a no-op
}

void TrackingGeometryVisitor::visitSurface(const Surface& /*surface*/) {
  // Default implementation is a no-op
}

void TrackingGeometryVisitor::visitLayer(const Layer& /*layer*/) {
  // Default implementation is a no-op
}

void TrackingGeometryVisitor::visitBoundarySurface(
    const BoundarySurfaceT<TrackingVolume>& /*boundary*/) {
  // Default implementation is a no-op
}

void TrackingGeometryMutableVisitor::visitVolume(TrackingVolume& /*volume*/) {
  // Default implementation is a no-op
}

void TrackingGeometryMutableVisitor::visitPortal(Portal& /*portal*/) {
  // Default implementation is a no-op
}

void TrackingGeometryMutableVisitor::visitSurface(Surface& /*surface*/) {
  // Default implementation is a no-op
}

void TrackingGeometryMutableVisitor::visitBoundarySurface(
    BoundarySurfaceT<TrackingVolume>& /*boundary*/) {
  // Default implementation is a no-op
}

void TrackingGeometryMutableVisitor::visitLayer(Layer& /*layer*/) {
  // Default implementation is a no-op
}

}  // namespace Acts
