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
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceVisitorConcept.hpp"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace Acts {

class ISurfaceMaterial;
class Surface;

namespace Experimental {
class DetectorVolume;
struct NavigationState;

/// A portal description between the detector volumes
///
/// It has a Surface representation for navigation and propagation
/// and guides from one volume to the next.
///
/// The surface can carry material to allow mapping onto
/// portal positions if required.
///
class Portal {
 public:
  /// Constructor from surface w/o portal links
  ///
  /// @param surface is the representing surface
  Portal(std::shared_ptr<RegularSurface> surface);

  /// The vector of attached volumes forward/backward, this is useful in the
  /// geometry building
  using AttachedDetectorVolumes =
      std::array<std::vector<std::shared_ptr<DetectorVolume>>, 2u>;

  /// Declare the DetectorVolume friend for portal setting
  friend class DetectorVolume;

  Portal() = delete;

  /// Const access to the surface representation
  const RegularSurface& surface() const;

  /// Non-const access to the surface reference
  RegularSurface& surface();

  /// @brief Visit all reachable surfaces of the detector
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be called with the represented surface
  template <SurfaceVisitor visitor_t>
  void visitSurface(visitor_t&& visitor) const {
    visitor(m_surface.get());
  }

  /// @brief Visit all reachable surfaces of the detector - non-const
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be called with the represented surface
  template <MutableSurfaceVisitor visitor_t>
  void visitMutableSurface(visitor_t&& visitor) {
    visitor(m_surface.get());
  }

  /// Update the current volume
  ///
  /// @param gctx is the Geometry context of this call
  /// @param nState [in,out] the navigation state for the volume to be updated
  ///
  void updateDetectorVolume(const GeometryContext& gctx,
                            NavigationState& nState) const noexcept(false);

  /// Set the geometry identifier (to the underlying surface)
  ///
  /// @param geometryId the geometry identifier to be assigned
  void assignGeometryId(const GeometryIdentifier& geometryId);

  /// Fuse with another portal, this one is kept
  ///
  /// @param aPortal is the first portal to fuse
  /// @param bPortal is the second portal to fuse
  ///
  /// @note this will combine the portal links from the both
  /// portals into a new one, it will throw an exception if the
  /// portals are not fusable
  ///
  /// @note if one portal carries material, it will be kept,
  /// however, if both portals carry material, an exception
  /// will be thrown and the portals are not fusable
  ///
  /// @note Both input portals become invalid, in that their update
  /// delegates and attached volumes are reset
  static std::shared_ptr<Portal> fuse(
      std::shared_ptr<Portal>& aPortal,
      std::shared_ptr<Portal>& bPortal) noexcept(false);

  /// Update the volume link
  ///
  /// @param dir the direction of the link
  /// @param portalNavigation is the navigation delegate
  /// @param attachedVolumes is the list of attached volumes for book keeping
  ///
  /// @note this overwrites the existing link
  void assignPortalNavigation(
      Direction dir, ExternalNavigationDelegate portalNavigation,
      std::vector<std::shared_ptr<DetectorVolume>> attachedVolumes);

  /// Update the volume link, w/o directive, i.e. it relies that there's only
  /// one remaining link to be set, throws an exception if that's not the case
  ///
  /// @param portalNavigation is the navigation delegate
  /// @param attachedVolumes is the list of attached volumes for book keeping
  ///
  /// @note this overwrites the existing link
  void assignPortalNavigation(ExternalNavigationDelegate portalNavigation,
                              std::vector<std::shared_ptr<DetectorVolume>>
                                  attachedVolumes) noexcept(false);

  // Access to the portal targets: opposite/along normal vector
  const std::array<ExternalNavigationDelegate, 2u>& portalNavigation() const;

  // Access to the attached volumes - non-const access
  AttachedDetectorVolumes& attachedDetectorVolumes();

 private:
  /// The surface representation of this portal
  std::shared_ptr<RegularSurface> m_surface;

  /// The portal targets along/opposite the normal vector
  std::array<ExternalNavigationDelegate, 2u> m_portalNavigation = {
      ExternalNavigationDelegate{}, ExternalNavigationDelegate{}};

  /// The portal attaches to the following volumes
  AttachedDetectorVolumes m_attachedVolumes;
};

}  // namespace Experimental
}  // namespace Acts
