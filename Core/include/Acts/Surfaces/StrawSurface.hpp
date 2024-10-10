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
#include "Acts/Surfaces/LineBounds.hpp"
#include "Acts/Surfaces/LineSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceConcept.hpp"

#include <cstddef>
#include <memory>
#include <string>

namespace Acts {

class DetectorElementBase;
struct Polyhedron;
class LineBounds;

///  @class StrawSurface
///
///  Class for a StrawSurface in the TrackingGeometry
///  to describe dirft tube and straw like detectors.
///
/// @image html LineSurface.png
///
class StrawSurface : public LineSurface {
  friend class Surface;

 protected:
  /// Constructor from Transform3 and bounds
  ///
  /// @param transform the transform that positions the straw in the global
  /// frame
  /// @param radius is the straw radius
  /// @param halez is the half length in z
  StrawSurface(const Transform3& transform, double radius, double halez);

  /// Constructor from Transform3 and a shared bounds object
  ///
  /// @param transform the transform that positions the straw in the global
  /// frame
  /// @param lbounds are the bounds describing the straw dimensions, can be
  /// optionally nullptr
  StrawSurface(const Transform3& transform,
               std::shared_ptr<const LineBounds> lbounds = nullptr);

  /// Constructor from DetectorElementBase : Element proxy
  ///
  /// @param lbounds are the bounds describing the straw dimensions, they must
  /// not be nullptr
  /// @param detelement for which this surface is (at least) one representation
  StrawSurface(const std::shared_ptr<const LineBounds>& lbounds,
               const DetectorElementBase& detelement);

  /// Copy constructor
  ///
  /// @param other is the source surface for copying
  StrawSurface(const StrawSurface& other);

  /// Copy constructor - with shift
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param other is the source cone surface
  /// @param shift is the additional transform applied after copying
  StrawSurface(const GeometryContext& gctx, const StrawSurface& other,
               const Transform3& shift);

 public:
  ~StrawSurface() override = default;
  StrawSurface() = delete;

  /// Assignment operator
  ///
  /// @param other is the source surface for copying
  StrawSurface& operator=(const StrawSurface& other);

  /// Return the surface type
  SurfaceType type() const final;

  /// Return properly formatted class name for screen output */
  std::string name() const final;

  /// Return a Polyhedron for the surfaces
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param quarterSegments is the number of segments used to describe curved
  /// segments in a quarter of the phi range. If it is 1, then only the extrema
  /// points in phi are inserted next to the segment corners.
  ///
  /// @return A list of vertices and a face/facett description of it
  Polyhedron polyhedronRepresentation(const GeometryContext& gctx,
                                      unsigned int quarterSegments) const final;
};

inline Surface::SurfaceType StrawSurface::type() const {
  return Surface::Straw;
}

inline std::string Acts::StrawSurface::name() const {
  return "Acts::StrawSurface";
}

static_assert(SurfaceConcept<StrawSurface>,
              "StrawSurface does not fulfill SurfaceConcept");

}  // namespace Acts
