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
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <algorithm>
#include <memory>
#include <utility>

namespace Acts {

/// @class NavigationLayer
///
/// Class to be used for gaps in Volumes as a navigational link.
/// Navigation Layers have a surface representation, but should usually never be
/// propagated to.
class NavigationLayer : public Layer {
 public:
  ///  Factory Constructor - the surface representation is given by pointer
  /// (ownership passed)
  ///
  /// @param sRepresentation is the representation for extrapolation
  /// @param thickness is the thickness for the binning
  static LayerPtr create(std::shared_ptr<const Surface> sRepresentation,
                         double thickness = 0.) {
    return LayerPtr(new NavigationLayer(std::move(sRepresentation), thickness));
  }

  /// Destructor
  ~NavigationLayer() override;

  /// The binning position method
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param bValue is the value for which the binning position is requested
  ///  - as default the center is given, but may be overloaded
  ///
  /// @return The return vector can be used for binning in a TrackingVolume
  Vector3 binningPosition(const GeometryContext& gctx,
                          BinningValue bValue) const final;

  /// Default Constructor - deleted
  NavigationLayer() = delete;

  /// Copy Constructor - deleted
  NavigationLayer(const NavigationLayer&) = delete;

  /// Assignment operator - deleted
  NavigationLayer& operator=(const NavigationLayer&) = delete;

  /// Transforms the layer into a Surface representation for extrapolation
  /// In general, extrapolation to a surface should be avoided
  const Surface& surfaceRepresentation() const final;

  // Non-const version
  Surface& surfaceRepresentation() final;

  /// Geometric isOnLayer() method
  /// using isOnSurface() with Layer specific tolerance
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param gp is the global position for the check
  /// @param boundaryTolerance is the boundary check directive
  ///
  /// @return boolean that indicates if the position is on surface
  bool isOnLayer(const GeometryContext& gctx, const Vector3& gp,
                 const BoundaryTolerance& boundaryTolerance =
                     BoundaryTolerance::None()) const final;

  /// Accept layer according to the following collection directives
  ///
  /// @param resolveSensitive is the prescription to find the sensitive surfaces
  /// @param resolveMaterial is the precription to find material surfaces
  /// @param resolvePassive is the prescription to find all passive surfaces
  ///
  /// @note navigation layers are never accepted
  ///
  /// @return a boolean whether the layer is accepted for processing
  bool resolve(bool resolveSensitive, bool resolveMaterial,
               bool resolvePassive) const final;

 protected:
  /// Private Constructor
  /// - this is called by the creat(args*) method
  /// passed spacer layer if needed
  ///
  /// @param surfaceRepresentation is the surface of the layer
  /// @param thickness ithe layer thickness
  NavigationLayer(std::shared_ptr<const Surface> surfaceRepresentation,
                  double thickness);

  /// for the navigation Volume the surface
  ///
  /// We will need to mutate this surface during the geometry building process,
  /// but the C++ type system has no const-correct way of expressing this.
  ///
  std::shared_ptr<const Surface> m_surfaceRepresentation;
};

inline const Surface& NavigationLayer::surfaceRepresentation() const {
  return (*m_surfaceRepresentation);
}

inline Surface& NavigationLayer::surfaceRepresentation() {
  return *(const_cast<Surface*>(m_surfaceRepresentation.get()));
}

inline Vector3 NavigationLayer::binningPosition(const GeometryContext& gctx,
                                                BinningValue bValue) const {
  return m_surfaceRepresentation->binningPosition(gctx, bValue);
}

inline bool NavigationLayer::isOnLayer(
    const GeometryContext& gctx, const Vector3& gp,
    const BoundaryTolerance& boundaryTolerance) const {
  return m_surfaceRepresentation->isOnSurface(gctx, gp, Vector3::Zero(),
                                              boundaryTolerance);
}

inline bool NavigationLayer::resolve(bool /*resolveSensitive*/,
                                     bool /*resolveMaterial*/,
                                     bool /*reolvePassive*/) const {
  return false;
}

}  // namespace Acts
