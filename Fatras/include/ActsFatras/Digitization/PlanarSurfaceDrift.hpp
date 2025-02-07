// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceError.hpp"

#include <array>
#include <functional>

namespace ActsFatras {

/// The PlanarSurfaceDrift takes an intersection in the nominal surface and
/// projects the ends into the readout surface, which can be at : -1, 0, 1
///
/// A Lorentz drift angle can be applied.
///
struct PlanarSurfaceDrift {
  /// Shorthand for a 2D segment
  using Segment2D = std::array<Acts::Vector2, 2>;

  /// Drift the full 3D segment onto a surface 2D readout plane
  ///
  /// @param gctx The current Geometry context
  /// @param surface The nominal intersection surface
  /// @param thickness The emulated module/depletion thickness
  /// @param pos The position in global coordinates
  /// @param dir The direction in global coordinates
  /// @param driftdir The drift direction in local (surface) coordinates
  /// @note a drift direction of (0,0,0) is drift to central plane
  ///       any other a drift direction with driftDir.z() != 0.
  ///       will result on a readout on either + 0.5*depletion
  ///       or -0.5*depletion
  ///
  /// @return a Segment on the readout surface @note without masking
  Segment2D toReadout(const Acts::GeometryContext& gctx,
                      const Acts::Surface& surface, double thickness,
                      const Acts::Vector3& pos, const Acts::Vector3& dir,
                      const Acts::Vector3& driftdir) const;
};

}  // namespace ActsFatras
