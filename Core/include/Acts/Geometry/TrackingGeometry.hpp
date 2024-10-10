// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/TrackingVolumeVisitorConcept.hpp"
#include "Acts/Surfaces/SurfaceVisitorConcept.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

namespace Acts {

class Layer;
class Surface;
class PerigeeSurface;
class IMaterialDecorator;
class TrackingVolume;

///  @class TrackingGeometry
///
///  The TrackingGeometry class is the owner of the constructed TrackingVolumes.
///
///  It enables both, a global search for an asociatedVolume
///  (respectively, if existing, a global search of an associated Layer or the
///  next associated Layer), such as a continuous navigation by BoundarySurfaces
///  between the confined TrackingVolumes.
class TrackingGeometry {
  /// Give the GeometryBuilder friend rights
  friend class TrackingGeometryBuilder;

 public:
  /// Constructor
  ///
  /// @param highestVolume is the world volume
  /// @param materialDecorator is a dediated decorator that can assign
  ///        surface or volume based material to the TrackingVolume
  /// @param hook Identifier hook to be applied to surfaces
  /// @param logger instance of a logger (defaulting to the "silent" one)
  TrackingGeometry(const std::shared_ptr<TrackingVolume>& highestVolume,
                   const IMaterialDecorator* materialDecorator = nullptr,
                   const GeometryIdentifierHook& hook = {},
                   const Logger& logger = getDummyLogger());

  /// Destructor
  ~TrackingGeometry();

  /// Access to the world volume
  /// @return plain pointer to the world volume
  const TrackingVolume* highestTrackingVolume() const;

  /// Access to the world volume
  /// @return shared pointer to the world volume
  std::shared_ptr<const TrackingVolume> highestTrackingVolumePtr() const;

  /// return the lowest tracking Volume
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param gp is the global position of the call
  ///
  /// @return plain pointer to the lowest TrackingVolume
  const TrackingVolume* lowestTrackingVolume(const GeometryContext& gctx,
                                             const Vector3& gp) const;

  /// Forward the associated Layer information
  ///
  /// @param gctx is the context for this request (e.g. alignment)
  /// @param gp is the global position of the call
  ///
  /// @return plain pointer to assocaiated layer
  const Layer* associatedLayer(const GeometryContext& gctx,
                               const Vector3& gp) const;

  /// @brief Visit all reachable surfaces
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor The callable. Will be called for each reachable surface
  /// that is found, a selection of the surfaces can be done in the visitor
  /// @param restrictToSensitives If true, only sensitive surfaces are visited
  ///
  /// @note If a context is needed for the visit, the visitor has to provide
  /// this, e.g. as a private member
  template <SurfaceVisitor visitor_t>
  void visitSurfaces(visitor_t&& visitor, bool restrictToSensitives) const {
    highestTrackingVolume()->template visitSurfaces<visitor_t>(
        std::forward<visitor_t>(visitor), restrictToSensitives);
  }

  /// @brief Visit all sensitive surfaces
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor The callable. Will be called for each sensitive surface
  /// that is found, a selection of the surfaces can be done in the visitor
  ///
  /// @note If a context is needed for the visit, the visitor has to provide
  /// this, e.g. as a private member
  template <SurfaceVisitor visitor_t>
  void visitSurfaces(visitor_t&& visitor) const {
    visitSurfaces(std::forward<visitor_t>(visitor), true);
  }

  /// @brief Visit all reachable tracking volumes
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor The callable. Will be called for each reachable volume
  /// that is found, a selection of the volumes can be done in the visitor
  ///
  /// @note If a context is needed for the visit, the visitor has to provide
  /// this, e.g. as a private member
  template <TrackingVolumeVisitor visitor_t>
  void visitVolumes(visitor_t&& visitor) const {
    highestTrackingVolume()->template visitVolumes<visitor_t>(
        std::forward<visitor_t>(visitor));
  }

  /// Search for a volume with the given identifier.
  ///
  /// @param id is the geometry identifier of the volume
  /// @retval nullptr if no such volume exists
  /// @retval pointer to the found volume otherwise.
  const TrackingVolume* findVolume(GeometryIdentifier id) const;

  /// Search for a surface with the given identifier.
  ///
  /// @param id is the geometry identifier of the surface
  /// @retval nullptr if no such surface exists
  /// @retval pointer to the found surface otherwise.
  const Surface* findSurface(GeometryIdentifier id) const;

  /// Access to the GeometryIdentifier - Surface association map
  const std::unordered_map<GeometryIdentifier, const Surface*>&
  geoIdSurfaceMap() const;

  /// Visualize a tracking geometry including substructure
  /// @param helper The visualization helper that implement the output
  /// @param gctx The geometry context
  /// @param viewConfig Global view config
  /// @param portalViewConfig View config for portals
  /// @param sensitiveViewConfig View configuration for sensitive surfaces
  void visualize(IVisualization3D& helper, const GeometryContext& gctx,
                 const ViewConfig& viewConfig = s_viewVolume,
                 const ViewConfig& portalViewConfig = s_viewPortal,
                 const ViewConfig& sensitiveViewConfig = s_viewSensitive) const;

 private:
  // the known world
  std::shared_ptr<TrackingVolume> m_world;
  // lookup containers
  std::unordered_map<GeometryIdentifier, const TrackingVolume*> m_volumesById;
  std::unordered_map<GeometryIdentifier, const Surface*> m_surfacesById;
};

}  // namespace Acts
