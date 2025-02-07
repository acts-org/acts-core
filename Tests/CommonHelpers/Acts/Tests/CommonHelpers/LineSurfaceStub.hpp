// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/LineSurface.hpp"

#include <limits>

namespace Acts::Test {

class LineSurfaceStub : public LineSurface {
 public:
  LineSurfaceStub() = delete;
  //
  LineSurfaceStub(const Transform3& htrans, double radius, double halfz)
      : GeometryObject(), LineSurface(htrans, radius, halfz) { /* nop */ }
  //
  LineSurfaceStub(const Transform3& htrans,
                  std::shared_ptr<const LineBounds> lbounds = nullptr)
      : GeometryObject(), LineSurface(htrans, std::move(lbounds)) { /*nop */ }
  //
  LineSurfaceStub(std::shared_ptr<const LineBounds> lbounds,
                  const DetectorElementBase& detelement)
      : GeometryObject(),
        LineSurface(std::move(lbounds), detelement) { /* nop */ }

  //
  LineSurfaceStub(const LineSurfaceStub& ls)
      : GeometryObject(), LineSurface(ls) { /* nop */ }

  LineSurfaceStub& operator=(const LineSurfaceStub& ls) = default;

  //
  LineSurfaceStub(const GeometryContext& gctx, const LineSurfaceStub& ls,
                  const Transform3& t)
      : GeometryObject(), LineSurface(gctx, ls, t) { /* nop */ }

  /// Return method for the Surface type to avoid dynamic casts
  SurfaceType type() const final { return Surface::Straw; }

  /// Simply return true to show object exists and is callable
  bool constructedOk() const { return true; }

  using Surface::normal;

  /// Return a Polyhedron for the surfaces
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param ingoredSegmeent is ignored for the srub
  ///
  /// @return A list of vertices and a face/facett description of it
  Polyhedron polyhedronRepresentation(const GeometryContext& /*gctx*/,
                                      unsigned int /*lseg*/) const final {
    return Polyhedron({}, {}, {});
  }
};
}  // namespace Acts::Test
