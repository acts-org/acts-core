// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Direction.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/Result.hpp"

#include <memory>

namespace Acts {

class RegularSurface;
class GeometryContext;
class TrackingVolume;
class CylinderSurface;
class PlaneSurface;
class DiscSurface;
class Surface;

class PortalLinkBase;

/// Exception thrown when portals cannot be merged
class PortalMergingException : public std::exception {
  const char* what() const noexcept override;
};

/// Exception thrown when portals cannot be fused
class PortalFusingException : public std::exception {
  const char* what() const noexcept override;
};

/// A portal connects two or more neighboring volumes. Each volume has a set of
/// portals that describes which volumes lie behind the portal in that
/// direction. Portals use associated portal links to perform lookups of target
/// volumes.
/// Each portal has two links, and a corresponding surface. One link is
/// associated with the direction along the surface's normal vector, and one
/// with the opposite direction.
class Portal {
 public:
  /// Constructor for a portal from a single link
  /// @param direction The direction of the link
  /// @param link The portal link
  Portal(Direction direction, std::unique_ptr<PortalLinkBase> link);

  /// Constructor for a portal from a surface and volume, where a trivial portal
  /// link is automatically constructed.
  /// @param direction The direction of the link
  /// @param surface The surface from which to create the portal link
  /// @param volume The volume this portal connects to in the @p direction
  ///               relative to the normal of @p surface.
  Portal(Direction direction, std::shared_ptr<RegularSurface> surface,
         TrackingVolume& volume);

  /// Constructor for a portal from two links. One of the links can be
  /// `nullptr`, but at least one of them needs to be set. If both are set, they
  /// need to be valid compatible links that can be fused.
  /// @param gctx The geometry context
  /// @param alongNormal The link along the normal of the surface
  /// @param oppositeNormal The link opposite to the normal of the
  Portal(const GeometryContext& gctx,
         std::unique_ptr<PortalLinkBase> alongNormal,
         std::unique_ptr<PortalLinkBase> oppositeNormal);

  /// Helper struct for the arguments to the portal constructor below using
  /// designated initializers.
  struct Arguments {
    /// Aggregate over a surface and a volume with optional semantics
    struct Link {
      Link() = default;
      /// Constructor from a surface and a volume
      Link(std::shared_ptr<RegularSurface> surfaceIn, TrackingVolume& volumeIn)
          : surface(std::move(surfaceIn)), volume(&volumeIn) {}

      /// The associated surface
      std::shared_ptr<RegularSurface> surface = nullptr;
      /// The associated volume
      TrackingVolume* volume = nullptr;
    };

    /// Entry for the link along normal
    /// Entry for the link opposite normal
    Link alongNormal{};
    Link oppositeNormal{};
  };

  /// Constructor that takes a geometry context and an rvalue reference to a
  /// helper struct from above. This pattern allows you to use designated
  /// initializers to construct this object like:
  /// ```cpp
  /// Portal{gctx, {.oppositeNormal = {cyl1, *vol1}}};
  /// Portal{gctx, {.alongNormal = {cyl2, *vol2}}};
  /// ```
  /// @param gctx The geometry context
  /// @param args The struct containing the arguments
  Portal(const GeometryContext& gctx, Arguments&& args);

  /// Fuse two portals together. Fusing is the combination of two portal links
  /// on the same logical surfaces. The actual surface instances can be
  /// different, as long as they are geometrically equivalent (within numerical
  /// precision). The resulting portal will have one portal along the shared
  /// surface's normal vector, and one opposite that vector.
  ///
  ///    portal1   portal2
  ///      +---+   +---+
  ///      |   |   |   |
  ///      |   |   |   |
  /// <----+   | + |   +---->
  ///      |   |   |   |
  ///      |   |   |   |
  ///      +---+   +---+
  ///
  /// @note The input portals need to have compatible link loadaout, e.g. one
  ///       portal needs to have the *along normal* slot filled, and the
  ///       otherone one needs to have the *opposite normal* slot filled. If
  ///       portals share a filled slot, the function throws an exception.
  /// @note This is a destructive operation on the portals involved
  /// @param gctx The geometry context
  /// @param aPortal The first portal
  /// @param bPortal The second portal
  /// @param logger The logger to push output to
  static Portal fuse(const GeometryContext& gctx, Portal& aPortal,
                     Portal& bPortal, const Logger& logger = getDummyLogger());

  /// Merge two adjacent portals with each other to produce a new portal that
  /// encompasses both inputs. It is the complementary operation to the fusing
  /// of portals. To be able to merge portals, the surfaces of their associated
  /// links need to be *mergeable*, and the portal links need to be compatible.
  /// This means that both portals need to have a link along the portal surface
  /// normal, opposite the normal, or both. If the equipped links are opposite
  /// relative to one another (e.g. one along one opposite), the function will
  /// throw an exception.
  ///
  ///         ^                     ^
  ///         |                     |
  ///  portal1|              portal2|
  /// +-------+-------+     +-------+-------+
  /// |               |  +  |               |
  /// +-------+-------+     +-------+-------+
  ///         |                     |
  ///         |                     |
  ///         v                     v
  ///
  /// @note This is a destructive operation on both portals, their
  ///       links will be moved to produce merged links, which can fail
  ///       if the portal links are not compatible
  /// @param gctx The geometry context
  /// @param aPortal The first portal
  /// @param bPortal The second portal
  /// @param direction The direction of the merge (e.g. along z)
  /// @param logger The logger to push output to
  static Portal merge(const GeometryContext& gctx, Portal& aPortal,
                      Portal& bPortal, BinningValue direction,
                      const Logger& logger = getDummyLogger());

  /// Resolve the volume for a 3D position and a direction
  /// The @p direction is used to select the right portal link, if it is set.
  /// In case no link is found in the specified direction, a `nullptr` is
  /// returned.
  /// @param gctx The geometry context
  /// @param position The 3D position
  /// @param direction The direction
  /// @return The target volume (can be `nullptr`)
  Result<const TrackingVolume*> resolveVolume(const GeometryContext& gctx,
                                              const Vector3& position,
                                              const Vector3& direction) const;

  /// Set a link on the portal into the slot specified by the direction.
  /// @note The surface associated with @p link must be logically equivalent
  ///       to the one of the link that's already set on the portal.
  /// @param gctx The geometry context
  /// @param direction The direction
  /// @param link The link to set
  void setLink(const GeometryContext& gctx, Direction direction,
               std::unique_ptr<PortalLinkBase> link);

  /// Helper function create a trivial portal link based on a surface.
  /// @param gctx The geometry context
  /// @param direction The direction of the link to create
  /// @param surface The surface
  /// @note The @p surface must be logically equivalent
  ///       to the one of the link that's already set on the portal.
  /// @param volume The target volume
  void setLink(const GeometryContext& gctx, Direction direction,
               std::shared_ptr<RegularSurface> surface, TrackingVolume& volume);

  /// Get the link associated with the @p direction. Can be null if the associated link is unset.
  /// @param direction The direction
  /// @return The link (can be null)
  const PortalLinkBase* getLink(Direction direction) const;

  /// Returns true if the portal is valid, that means it has at least one
  /// non-null link associated.Portals can be in an invalid state after they get
  /// merged or fused with other portals.
  /// @return True if the portal is valid
  bool isValid() const;

  /// Create and attach a trivial portal link to the empty slot of this portal
  /// @param volume The target volume to connect to
  void fill(TrackingVolume& volume);

  /// Access the portal surface that is shared between the two links
  /// @return The portal surface
  const RegularSurface& surface() const;

  /// Access the portal surface that is shared between the two links
  /// @return The portal surface
  RegularSurface& surface();

 private:
  /// Helper to check surface equivalence without checking material status. This
  /// is needed because we allow fusing portals with surfaces that are
  /// equivalent but one of them has material while the other does not. The
  /// normal surface comparison would determine these surfaces as not
  /// equivalent.
  /// @param gctx The geometry context
  /// @param a The first surface
  /// @param b The second surface
  /// @return True if the surfaces are equivalent
  static bool isSameSurface(const GeometryContext& gctx, const Surface& a,
                            const Surface& b);

  std::shared_ptr<RegularSurface> m_surface;

  std::unique_ptr<PortalLinkBase> m_alongNormal;
  std::unique_ptr<PortalLinkBase> m_oppositeNormal;
};

}  // namespace Acts
