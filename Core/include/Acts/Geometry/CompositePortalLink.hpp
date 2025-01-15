// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Geometry/PortalLinkBase.hpp"

#include <iosfwd>

#include <boost/container/small_vector.hpp>

namespace Acts {

class GridPortalLink;
class Surface;

/// Composite portal links can graft together other portal link instances, for
/// example grids that could not be merged due to invalid binnings.
///
/// ```
/// +-------+      +-------+
/// |       |      |       |
/// |       |      |       |
/// |       |      |       |
/// +-------+      |       |
/// |       |      |       |
/// |       |  +   +-------+
/// |       |      |       |
/// +-------+      |       |
/// |       |      |       |
/// |       |      +-------+
/// |       |      |       |
/// +-------+      +-------+
/// ```
///
/// During resolution, it will consult each of it's children and return
/// the result on the first surface where the lookup position is within
/// bounds.
class CompositePortalLink final : public PortalLinkBase {
 public:
  /// Construct a composite portal from two arbitrary other portal links. The
  /// only requirement is that the portal link surfaces are mergeable.
  /// @param a The first portal link
  /// @param b The second portal link
  /// @param direction The binning direction
  /// @param flatten If true, the composite will flatten any nested composite
  CompositePortalLink(std::unique_ptr<PortalLinkBase> a,
                      std::unique_ptr<PortalLinkBase> b,
                      AxisDirection direction, bool flatten = true);

  /// Construct a composite portal from any number of arbitrary other portal
  /// links. The only requirement is that the portal link surfaces are
  /// mergeable.
  /// @param links The portal links
  /// @param direction The binning direction
  /// @param flatten If true, the composite will flatten any nested composite
  CompositePortalLink(std::vector<std::unique_ptr<PortalLinkBase>> links,
                      AxisDirection direction, bool flatten = true);

  /// Print the composite portal link
  /// @param os The output stream
  void toStream(std::ostream& os) const override;

  /// Resolve the volume for a 2D position
  /// @note This will transform the position to global coordinates before
  ///       consulting its children.
  /// @note @p position is assumed to be on surface
  /// @param gctx The geometry context
  /// @param position The 2D position
  /// @param tolerance The on-surface tolerance
  Result<const TrackingVolume*> resolveVolume(
      const GeometryContext& gctx, const Vector2& position,
      double tolerance = s_onSurfaceTolerance) const override;

  /// Resolve the volume for a 3D position
  /// @note @p position is assumed to be on surface
  /// @param gctx The geometry context
  /// @param position The 3D position
  /// @param tolerance The tolerance
  Result<const TrackingVolume*> resolveVolume(
      const GeometryContext& gctx, const Vector3& position,
      double tolerance = s_onSurfaceTolerance) const override;

  /// Get the depth of the composite tree
  /// @return The depth
  std::size_t depth() const;

  /// Get the number of children
  /// @return The number of children
  std::size_t size() const;

  /// (Potentially) create a grid portal link that represents this composite
  /// portal link.
  /// @note This only works, if the composite is **flat** and only contains
  ///       **trivial portal links**. If these preconditions are not met, this
  ///       function returns a nullptr.
  /// @param gctx The geometry context
  /// @param logger The logger
  /// @return The grid portal link
  std::unique_ptr<GridPortalLink> makeGrid(const GeometryContext& gctx,
                                           const Logger& logger) const;

 private:
  boost::container::small_vector<std::unique_ptr<PortalLinkBase>, 4>
      m_children{};

  AxisDirection m_direction;
};

}  // namespace Acts
