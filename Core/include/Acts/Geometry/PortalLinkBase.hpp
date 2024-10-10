// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>

namespace Acts {

class RegularSurface;
class TrackingVolume;
class GeometryContext;

/// PortalLinkBase is the abstract base class for all portal links.
/// A portal link is a mapping between a surface and a point on the surface and
/// a destination tracking volum.
/// The derived classes implement different ways to resolve a volume
class PortalLinkBase {
 protected:
  /// Constructor from a surface. This constructor is only
  /// called from derived classes
  /// @param surface The surface
  explicit PortalLinkBase(std::shared_ptr<RegularSurface> surface)
      : m_surface(std::move(surface)) {
    if (!m_surface) {
      throw std::invalid_argument("Surface pointer must not be null");
    }
  }

 public:
  /// Virtual destructor in case the object is held as a derived
  virtual ~PortalLinkBase() = default;

  /// Resolve a volume given a global position. Depending on the derived class,
  /// the global position might be converted to a local position before lookup.
  /// @param gctx The geometry context
  /// @param position The global position
  /// @param tolerance The tolerance for the lookup
  ///
  /// @return The tracking volume or null if no connection was found
  virtual Result<const TrackingVolume*> resolveVolume(
      const GeometryContext& gctx, const Vector3& position,
      double tolerance = s_onSurfaceTolerance) const = 0;

  /// Resolve a volume given a local position. The local position is assumed to
  /// be on surface.
  /// @param gctx The geometry context
  /// @param position The local position
  /// @param tolerance The tolerance for the lookup
  ///
  /// @return The tracking volume or null if no connection was found
  virtual Result<const TrackingVolume*> resolveVolume(
      const GeometryContext& gctx, const Vector2& position,
      double tolerance = s_onSurfaceTolerance) const = 0;

  //// Merge two portal link into a single one. The merge can resolve
  /// combinations of difference derived classes, and will try to flatten and
  /// deep merge given links if possible.
  /// @param a The first portal link
  /// @param b The second portal link
  /// @param direction The binning direction in which to merge. Valid values are
  ///                  depend on the surface types associated with the links.
  /// @param logger The logger to use for messages
  /// @return The merged portal link
  static std::unique_ptr<PortalLinkBase> merge(
      std::unique_ptr<PortalLinkBase> a, std::unique_ptr<PortalLinkBase> b,
      BinningValue direction, const Logger& logger = getDummyLogger());

  /// Stream output function
  /// @param os The output stream
  virtual void toStream(std::ostream& os) const = 0;

  /// Stream output operator
  /// @param os The output stream
  /// @param link The portal link
  friend std::ostream& operator<<(std::ostream& os,
                                  const PortalLinkBase& link) {
    link.toStream(os);
    return os;
  }

  /// Getter for the associated surface
  /// @return The surface
  const RegularSurface& surface() const { return *m_surface; }

  /// Setter for the surface
  /// @param surface The surface
  void setSurface(std::shared_ptr<RegularSurface> surface) {
    m_surface = std::move(surface);
  }

  /// Getter for the underlying shared pointer
  /// @return The shared pointer to the surface
  const std::shared_ptr<RegularSurface>& surfacePtr() const {
    return m_surface;
  }

 protected:
  /// Helper function to check a number of preconditions before merging is
  /// executed.
  static void checkMergePreconditions(const PortalLinkBase& a,
                                      const PortalLinkBase& b,
                                      BinningValue direction);

  std::shared_ptr<RegularSurface> m_surface;
};

}  // namespace Acts
