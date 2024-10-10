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
#include "Acts/Geometry/Polyhedron.hpp"
#include "Acts/Surfaces/InfiniteBounds.hpp"
#include "Acts/Surfaces/LineSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceConcept.hpp"

#include <cstddef>
#include <iosfwd>
#include <string>

namespace Acts {

/// @class PerigeeSurface
///
/// Class describing the Line to which the Perigee refers to.
/// The Surface axis is fixed to be the z-axis of the Tracking frame.
/// It inherits from StraingLineSurface.
///
/// @image html LineSurface.png
class PerigeeSurface : public LineSurface {
  friend class Surface;

 protected:
  /// Constructor from GlobalPosition
  ///
  /// @param gp position where the perigee is centered
  PerigeeSurface(const Vector3& gp);

  /// Constructor with a Transform - needed for tilt
  ///
  /// @param transform is the transform for position and tilting
  PerigeeSurface(const Transform3& transform);

  /// Copy constructor
  ///
  /// @param other is the source surface to be copied
  PerigeeSurface(const PerigeeSurface& other);

  /// Copy constructor - with shift
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param other is the source cone surface
  /// @param shift is the additional transform applied after copying
  PerigeeSurface(const GeometryContext& gctx, const PerigeeSurface& other,
                 const Transform3& shift);

 public:
  /// Destructor - defaulted
  ~PerigeeSurface() override = default;

  /// Default Constructor - deleted
  PerigeeSurface() = delete;

  /// Assignment operator
  ///
  /// @param other is the source surface to be assigned
  PerigeeSurface& operator=(const PerigeeSurface& other);

  /// Return the surface type
  SurfaceType type() const final;

  /// Return properly formatted class name for screen output */
  std::string name() const final;

  /// Return a Polyhedron for the surfaces
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param ingoreSegments is an ignored parameter
  ///
  /// @return A list of vertices and a face/facett description of it
  Polyhedron polyhedronRepresentation(const GeometryContext& gctx,
                                      unsigned int ingoreSegments) const final;

 protected:
  /// Output Method for std::ostream
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param sl is the ostream to be dumped into
  ///
  /// @return ostreamn object which was streamed into
  std::ostream& toStreamImpl(const GeometryContext& gctx,
                             std::ostream& sl) const final;
};

static_assert(SurfaceConcept<PerigeeSurface>,
              "PerigeeSurface does not fulfill SurfaceConcept");

}  // namespace Acts
