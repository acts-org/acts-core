// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Geometry/TrackingGeometry.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <cstddef>

Acts::TrackingGeometry::TrackingGeometry(
    const MutableTrackingVolumePtr& highestVolume,
    const IMaterialDecorator* materialDecorator,
    const GeometryIdentifierHook& hook, const Logger& logger)
    : m_world(highestVolume) {
  // Close the geometry: assign geometryID and successively the material
  std::size_t volumeID = 0;
  highestVolume->closeGeometry(materialDecorator, m_volumesById, volumeID, hook,
                               logger);
  m_volumesById.rehash(0);
  // fill surface lookup container
  m_world->visitSurfaces([this](const Acts::Surface* srf) {
    if (srf != nullptr) {
      m_surfacesById[srf->geometryId()] = srf;
    }
  });
  m_surfacesById.rehash(0);
}

Acts::TrackingGeometry::~TrackingGeometry() = default;

const Acts::TrackingVolume* Acts::TrackingGeometry::lowestTrackingVolume(
    const GeometryContext& gctx, const Acts::Vector3& gp) const {
  return m_world->lowestTrackingVolume(gctx, gp, s_onSurfaceTolerance);
}

const Acts::TrackingVolume* Acts::TrackingGeometry::highestTrackingVolume()
    const {
  return m_world.get();
}

std::shared_ptr<const Acts::TrackingVolume>
Acts::TrackingGeometry::highestTrackingVolumePtr() const {
  return m_world;
}

const Acts::Layer* Acts::TrackingGeometry::associatedLayer(
    const GeometryContext& gctx, const Acts::Vector3& gp) const {
  const TrackingVolume* lowestVol = lowestTrackingVolume(gctx, gp);
  if (lowestVol == nullptr) {
    return nullptr;
  }
  return lowestVol->associatedLayer(gctx, gp);
}

const Acts::TrackingVolume* Acts::TrackingGeometry::findVolume(
    GeometryIdentifier id) const {
  auto vol = m_volumesById.find(id);
  if (vol == m_volumesById.end()) {
    return nullptr;
  }
  return vol->second;
}

const Acts::Surface* Acts::TrackingGeometry::findSurface(
    GeometryIdentifier id) const {
  auto srf = m_surfacesById.find(id);
  if (srf == m_surfacesById.end()) {
    return nullptr;
  }
  return srf->second;
}

const std::unordered_map<Acts::GeometryIdentifier, const Acts::Surface*>&
Acts::TrackingGeometry::geoIdSurfaceMap() const {
  return m_surfacesById;
}

void Acts::TrackingGeometry::visualize(
    IVisualization3D& helper, const GeometryContext& gctx,
    const ViewConfig& viewConfig, const ViewConfig& portalViewConfig,
    const ViewConfig& sensitiveViewConfig) const {
  highestTrackingVolume()->visualize(helper, gctx, viewConfig, portalViewConfig,
                                     sensitiveViewConfig);
}
