// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/TrackingGeometry.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingGeometryVisitor.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/ProtoVolumeMaterial.hpp"
#include "Acts/Surfaces/Surface.hpp"

#include <cstddef>

namespace Acts {

class Gen1GeometryClosureVisitor : public TrackingGeometryMutableVisitor {
 public:
  Gen1GeometryClosureVisitor(const Logger& logger,
                             const IMaterialDecorator* materialDecorator,
                             const GeometryIdentifierHook& hook)
      : m_logger(&logger),
        m_materialDecorator(materialDecorator),
        m_hook(&hook) {
    ACTS_VERBOSE("Creating Gen1GeometryClosureVisitor");
  }

  const Logger& logger() const { return *m_logger; }

  void visitVolume(TrackingVolume& volume) override {
    ACTS_DEBUG("Volume: " << volume.volumeName());

    // Increment the volume ID for this volume
    m_volumeID = GeometryIdentifier().withVolume(m_volumeID.volume() + 1);
    // Reset boundary id for this volume
    m_iboundary = 0;
    // Reset layer id for this volume
    m_ilayer = 0;

    // assign the Volume ID to the volume itself
    ACTS_VERBOSE("~> volumeID: " << m_volumeID);
    volume.assignGeometryId(m_volumeID);

    // assign the material if you have a decorator
    if (m_materialDecorator != nullptr) {
      ACTS_VERBOSE("Decorating volume " << volume.volumeName()
                                        << " with material");
      m_materialDecorator->decorate(volume);
    }
    if (volume.volumeMaterial() == nullptr &&
        volume.motherVolume() != nullptr &&
        volume.motherVolume()->volumeMaterial() != nullptr) {
      auto protoMaterial = dynamic_cast<const ProtoVolumeMaterial*>(
          volume.motherVolume()->volumeMaterial());
      if (protoMaterial == nullptr) {
        volume.assignVolumeMaterial(volume.motherVolume()->volumeMaterialPtr());
      }
    }
  }

  void visitBoundarySurface(
      BoundarySurfaceT<TrackingVolume>& boundary) override {
    ACTS_DEBUG("BoundarySurface: " << boundary.surfaceRepresentation().name());
    // get the intersection solution
    auto& bSurface = boundary.surfaceRepresentation();
    // create the boundary surface id
    m_iboundary += 1;
    auto boundaryID = GeometryIdentifier(m_volumeID).withBoundary(m_iboundary);
    ACTS_VERBOSE("~> boundaryID: " << boundaryID);
    // now assign to the boundary surface
    auto& mutableBSurface = *(const_cast<RegularSurface*>(&bSurface));

    // assign the boundary ID to the surface
    ACTS_VERBOSE("~> assigning boundaryID: " << boundaryID);
    mutableBSurface.assignGeometryId(boundaryID);

    // Assign material if you have a decorator
    if (m_materialDecorator != nullptr) {
      ACTS_VERBOSE("Decorating boundary surface " << bSurface.name()
                                                  << " with material");
      m_materialDecorator->decorate(mutableBSurface);
    }
  }

  void visitLayer(Layer& layer) override {
    ACTS_DEBUG("Close Layer");
    // create the layer identification
    m_ilayer += 1;
    auto layerID = GeometryIdentifier(m_volumeID).withLayer(m_ilayer);
    ACTS_VERBOSE("~> layerID: " << layerID);

    // now close the geometry
    layer.closeGeometry(m_materialDecorator, layerID, *m_hook, *m_logger);
  }

  const Logger* m_logger;
  GeometryIdentifier m_volumeID;
  GeometryIdentifier::Value m_iboundary = 0;
  GeometryIdentifier::Value m_ilayer = 0;
  const IMaterialDecorator* m_materialDecorator = nullptr;
  const GeometryIdentifierHook* m_hook = nullptr;

  std::unordered_map<GeometryIdentifier, const TrackingVolume*> m_volumesById{};
  std::unordered_map<GeometryIdentifier, const Surface*> m_surfacesById{};
};

TrackingGeometry::TrackingGeometry(
    const MutableTrackingVolumePtr& highestVolume,
    const IMaterialDecorator* materialDecorator,
    const GeometryIdentifierHook& hook, const Logger& logger, bool close)
    : m_world(highestVolume) {
  if (close) {
    Gen1GeometryClosureVisitor visitor{logger, materialDecorator, hook};
    apply(visitor);
  }

  class Visitor : public TrackingGeometryVisitor {
   public:
    void visitVolume(const TrackingVolume& volume) override {
      auto [it, inserted] = m_volumesById.emplace(volume.geometryId(), &volume);
      if (!inserted) {
        std::stringstream ss;
        ss << "Duplicate volume ID: " << volume.geometryId();
        throw std::invalid_argument(ss.str());
      }
    }
    void visitSurface(const Surface& surface) override {
      if (surface.geometryId() == GeometryIdentifier{}) {
        throw std::invalid_argument("Surface has no geometry ID");
      }
      //@TODO: Why not use all of them?
      if (surface.geometryId().sensitive() != 0) {
        auto [it, inserted] =
            m_surfacesById.emplace(surface.geometryId(), &surface);
        if (!inserted) {
          std::stringstream ss;
          ss << "Duplicate surface ID: " << surface.geometryId();
          throw std::invalid_argument(ss.str());
        }
      }
    }

    std::unordered_map<GeometryIdentifier, const TrackingVolume*>
        m_volumesById{};
    std::unordered_map<GeometryIdentifier, const Surface*> m_surfacesById{};
  };
  Visitor mapVisitor;
  apply(mapVisitor);
  m_volumesById = std::move(mapVisitor.m_volumesById);
  m_surfacesById = std::move(mapVisitor.m_surfacesById);

  ACTS_DEBUG("TrackingGeometry created with "
             << m_volumesById.size() << " volumes and " << m_surfacesById.size()
             << " sensitive surfaces");

  m_volumesById.rehash(0);
  m_surfacesById.rehash(0);
}

TrackingGeometry::~TrackingGeometry() = default;

const TrackingVolume* TrackingGeometry::lowestTrackingVolume(
    const GeometryContext& gctx, const Vector3& gp) const {
  return m_world->lowestTrackingVolume(gctx, gp, s_onSurfaceTolerance);
}

const TrackingVolume* TrackingGeometry::highestTrackingVolume() const {
  return m_world.get();
}

TrackingVolume* TrackingGeometry::highestTrackingVolume() {
  return m_world.get();
}

std::shared_ptr<const TrackingVolume>
TrackingGeometry::highestTrackingVolumePtr() const {
  return m_world;
}

const Layer* TrackingGeometry::associatedLayer(const GeometryContext& gctx,
                                               const Vector3& gp) const {
  const TrackingVolume* lowestVol = lowestTrackingVolume(gctx, gp);
  if (lowestVol == nullptr) {
    return nullptr;
  }
  return lowestVol->associatedLayer(gctx, gp);
}

const TrackingVolume* TrackingGeometry::findVolume(
    GeometryIdentifier id) const {
  auto vol = m_volumesById.find(id);
  if (vol == m_volumesById.end()) {
    return nullptr;
  }
  return vol->second;
}

const Surface* TrackingGeometry::findSurface(GeometryIdentifier id) const {
  auto srf = m_surfacesById.find(id);
  if (srf == m_surfacesById.end()) {
    return nullptr;
  }
  return srf->second;
}

const std::unordered_map<GeometryIdentifier, const Surface*>&
TrackingGeometry::geoIdSurfaceMap() const {
  return m_surfacesById;
}

void TrackingGeometry::visualize(IVisualization3D& helper,
                                 const GeometryContext& gctx,
                                 const ViewConfig& viewConfig,
                                 const ViewConfig& portalViewConfig,
                                 const ViewConfig& sensitiveViewConfig) const {
  highestTrackingVolume()->visualize(helper, gctx, viewConfig, portalViewConfig,
                                     sensitiveViewConfig);
}

void TrackingGeometry::apply(TrackingGeometryVisitor& visitor) const {
  highestTrackingVolume()->apply(visitor);
}

void TrackingGeometry::apply(TrackingGeometryMutableVisitor& visitor) {
  highestTrackingVolume()->apply(visitor);
}

}  // namespace Acts
