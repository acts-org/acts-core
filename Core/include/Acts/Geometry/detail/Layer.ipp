// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <limits>
#include <map>

namespace Acts {

inline const SurfaceArray* Layer::surfaceArray() const {
  return m_surfaceArray.get();
}

inline SurfaceArray* Layer::surfaceArray() {
  return const_cast<SurfaceArray*>(m_surfaceArray.get());
}

inline double Layer::thickness() const {
  return m_layerThickness;
}

inline LayerType Layer::layerType() const {
  return m_layerType;
}

inline const TrackingVolume* Layer::trackingVolume() const {
  return m_trackingVolume;
}

inline void Layer::encloseTrackingVolume(const TrackingVolume& tvol) {
  m_trackingVolume = &(tvol);
}

inline const Volume* Layer::representingVolume() const {
  return m_representingVolume.get();
}

inline const Layer* Layer::nextLayer(const GeometryContext& /*gctx*/,
                                     const Vector3& position,
                                     const Vector3& direction) const {
  // no binutility -> no chance to find out the direction
  if (m_nextLayerUtility == nullptr) {
    return nullptr;
  }
  return (m_nextLayerUtility->nextDirection(position, direction) < 0)
             ? m_nextLayers.first
             : m_nextLayers.second;
}

inline bool Layer::resolve(bool resolveSensitive, bool resolveMaterial,
                           bool resolvePassive) const {
  if (resolvePassive) {
    return true;
  }
  if (resolveSensitive && m_surfaceArray) {
    return true;
  }
  if (resolveMaterial &&
      (m_ssSensitiveSurfaces > 1 || m_ssApproachSurfaces > 1 ||
       (surfaceRepresentation().surfaceMaterial() != nullptr))) {
    return true;
  }
  return false;
}

inline bool Layer::isOnLayer(const GeometryContext& gctx,
                             const Vector3& position,
                             const BoundaryTolerance& boundaryTolerance) const {
  if (m_representingVolume != nullptr) {
    return m_representingVolume->inside(position);
  }
  return surfaceRepresentation().isOnSurface(gctx, position, Vector3::Zero(),
                                             boundaryTolerance);
}

}  // namespace Acts
