// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Detector/DetectorVolumeVisitorConcept.hpp"
#include "Acts/Detector/Portal.hpp"
#include "Acts/Detector/PortalGenerators.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Navigation/NavigationState.hpp"
#include "Acts/Surfaces/BoundaryTolerance.hpp"
#include "Acts/Surfaces/SurfaceVisitorConcept.hpp"
#include "Acts/Utilities/BoundingBox.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

class Surface;
class IVolumeMaterial;
class VolumeBounds;

namespace Experimental {

class DetectorVolume;
class Detector;

/// A detector volume description which can be:
///
/// @note A detector volume holds non-const objects internally
/// that are allowed to be modified as long as the geometry
/// is not yet closed. Using this, material can be attached,
/// and GeometryIdentifier can be set at construction time.
///
/// @note The construction of DetectorVolumes is done via a dedicated
/// factory, this is necessary as then the shared_ptr is non-weak and it
/// can be registered in the portal generator for further geometry processing.
///
/// @note Navigation is always done by plain pointers, while
/// object ownership is done by shared/unique pointers.
class DetectorVolume : public std::enable_shared_from_this<DetectorVolume> {
 public:
  using BoundingBox =
      Acts::AxisAlignedBoundingBox<Acts::Experimental::DetectorVolume,
                                   Acts::ActsScalar, 3>;

  friend class DetectorVolumeFactory;

  /// Nested object store that holds the internal (non-const),
  /// reference counted objects and provides an external
  /// (const raw pointer) access
  ///
  /// @tparam internal_type is the internal storage representation,
  /// has to comply with std::shared_ptr semantics
  ///
  template <typename internal_type>
  struct ObjectStore {
    /// The internal storage vector
    std::vector<internal_type> internal = {};

    /// The external storage vector, const raw pointer
    std::vector<const typename internal_type::element_type*> external = {};

    /// Store constructor
    ///
    /// @param objects are the ones copied into the internal store
    ObjectStore(std::vector<internal_type> objects)
        : internal(std::move(objects)) {
      external = unpack_shared_const_vector(internal);
    }

    ObjectStore() = default;
  };

 protected:
  /// Create a detector volume - with surfaces and/or inserted volumes
  ///
  /// @param gctx the geometry context while building - for future contextual store
  /// @param name the volume name
  /// @param transform the transform defining the volume position
  /// @param bounds the volume bounds
  /// @param surfaces are the contained surfaces of this volume
  /// @param volumes are the contains volumes of this volume
  /// @param externalNavigation is a Delegate to find the associated volume
  /// @param internalNavigation the navigation state updator for surfaces/portals
  ///
  /// @note throws exception if misconfigured: no bounds
  /// @note throws exception if ghe portal general or navigation
  ///       state updator delegates are not connected
  DetectorVolume(const GeometryContext& gctx, std::string name,
                 const Transform3& transform,
                 std::shared_ptr<VolumeBounds> bounds,
                 std::vector<std::shared_ptr<Surface>> surfaces,
                 std::vector<std::shared_ptr<DetectorVolume>> volumes,
                 ExternalNavigationDelegate externalNavigation,
                 InternalNavigationDelegate internalNavigation) noexcept(false);

  /// Create a detector volume - empty/gap volume constructor
  ///
  /// @param gctx the geometry context while building - for future contextual store
  /// @param name the volume name
  /// @param transform the transform defining the volume position
  /// @param bounds the volume bounds
  /// @param internalNavigation the navigation state updator for surfaces/portals
  ///
  /// @note throws exception if misconfigured: no bounds
  /// @note throws exception if ghe portal general or navigation
  ///       state updator delegates are not connected
  DetectorVolume(const GeometryContext& gctx, std::string name,
                 const Transform3& transform,
                 std::shared_ptr<VolumeBounds> bounds,
                 InternalNavigationDelegate internalNavigation) noexcept(false);

  /// Factory method for producing memory managed instances of DetectorVolume.
  ///
  /// @note This is called by the @class DetectorVolumeFactory
  static std::shared_ptr<DetectorVolume> makeShared(
      const GeometryContext& gctx, std::string name,
      const Transform3& transform, std::shared_ptr<VolumeBounds> bounds,
      std::vector<std::shared_ptr<Surface>> surfaces,
      std::vector<std::shared_ptr<DetectorVolume>> volumes,
      ExternalNavigationDelegate externalNavigation,
      InternalNavigationDelegate internalNavigation);

  /// Factory method for producing memory managed instances of DetectorVolume.
  ///
  /// @note This is called by the @class DetectorVolumeFactory
  static std::shared_ptr<DetectorVolume> makeShared(
      const GeometryContext& gctx, std::string name,
      const Transform3& transform, std::shared_ptr<VolumeBounds> bounds,
      InternalNavigationDelegate internalNavigation);

 public:
  /// Retrieve a @c std::shared_ptr for this surface (non-const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior (but most likely implemented as a @c bad_weak_ptr
  ///       exception), in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<DetectorVolume> getSharedPtr();

  /// Retrieve a @c std::shared_ptr for this surface (const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior, but most likely implemented as a @c bad_weak_ptr
  ///       exception, in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<const DetectorVolume> getSharedPtr() const;

  /// Const access to the transform
  ///
  /// @param gctx the geometry context
  ///
  /// @note the geometry context is currently ignored, but
  ///       is a placeholder for eventually misaligned volumes
  ///
  /// @return const reference to the contextual transform
  const Transform3& transform(
      const GeometryContext& gctx = GeometryContext()) const;

  /// Const access to the center
  ///
  /// @param gctx the geometry context
  ///
  /// @note the geometry context is currently ignored, but
  ///       is a placeholder for eventually misaligned volumes
  ///
  /// @return a contextually created center
  Vector3 center(const GeometryContext& gctx = GeometryContext()) const;

  /// Const access to the volume bounds
  ///
  /// @return const reference to the volume bounds object
  const VolumeBounds& volumeBounds() const;

  /// Check if a point is inside this volume. Subvolumes will not be checked.
  ///
  /// @param gctx the geometry context
  /// @param position the position for the inside check
  ///
  /// @return a bool to indicate inside/outside
  bool inside(const GeometryContext& gctx, const Vector3& position) const;

  /// Check if a point is exclusively inside this volume i.e. this point is not
  /// inside a subvolume.
  ///
  /// @param gctx the geometry context
  /// @param position the position for the inside check
  ///
  /// @return a bool to indicate inside/outside
  bool exclusivelyInside(const GeometryContext& gctx,
                         const Vector3& position) const;

  /// The Extent for this volume
  ///
  /// @param gctx is the geometry context
  /// @param nseg is the number of segments to approximate
  ///
  /// @return an Extent object
  Extent extent(const GeometryContext& gctx, std::size_t nseg = 1) const;

  /// Initialize/update the navigation status in this environment
  ///
  /// This method calls:
  ///
  /// - the local navigation delegate for candidate surfaces
  /// - the portal navigation delegate for candidate exit portals
  /// - set the current detector volume
  ///
  /// @param gctx is the current geometry context
  /// @param nState [in,out] is the detector navigation state to be updated
  ///
  void updateNavigationState(const GeometryContext& gctx,
                             NavigationState& nState) const;

  /// Non-const access to the portals
  ///
  /// @return the portal shared pointer store
  std::vector<std::shared_ptr<Portal>>& portalPtrs();

  /// Non-const access to the surfaces
  ///
  /// @return the surfaces shared pointer store
  std::vector<std::shared_ptr<Surface>>& surfacePtrs();

  /// Non-const access to the volumes
  ///
  /// @return the volumes shared pointer store
  std::vector<std::shared_ptr<DetectorVolume>>& volumePtrs();

  /// Const access to the detector portals
  ///
  /// @note an empty vector indicates a container volume
  /// that has not been properly connected
  ///
  /// @return a vector to const Portal raw pointers
  const std::vector<const Portal*>& portals() const;

  /// Const access to the surfaces
  ///
  /// @note an empty vector indicates either gap volume
  /// or container volume, a non-empty vector indicates
  /// a layer volume.
  ///
  /// @return a vector to const Surface raw pointers
  const std::vector<const Surface*>& surfaces() const;

  /// Const access to sub volumes
  ///
  /// @note and empty vector indicates this is either a
  /// gap volume or a layer volume, in any case it means
  /// the volume is on navigation level and the portals
  /// need to be connected
  ///
  /// @return a vector to const DetectorVolume raw pointers
  const std::vector<const DetectorVolume*>& volumes() const;

  /// Const access to the detector volume updator
  const ExternalNavigationDelegate& externalNavigation() const;

  /// @brief Visit all reachable surfaces of the detector
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be called for each found surface,
  /// it will be handed down to contained volumes and portals
  template <SurfaceVisitor visitor_t>
  void visitSurfaces(visitor_t&& visitor) const {
    for (const auto& s : surfaces()) {
      visitor(s);
    }
    for (const auto& p : portals()) {
      p->visitSurface(std::forward<visitor_t>(visitor));
    }
    for (const auto& v : volumes()) {
      v->visitSurfaces(std::forward<visitor_t>(visitor));
    }
  }

  /// @brief Visit all reachable surfaces of the detector - non-const
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be called for each found surface,
  /// it will be handed down to contained volumes and portals
  template <MutableSurfaceVisitor visitor_t>
  void visitMutableSurfaces(visitor_t&& visitor) {
    for (auto& s : surfacePtrs()) {
      visitor(s.get());
    }
    for (auto& p : portalPtrs()) {
      p->visitMutableSurface(std::forward<visitor_t>(visitor));
    }
    for (auto& v : volumePtrs()) {
      v->visitMutableSurfaces(std::forward<visitor_t>(visitor));
    }
  }

  /// @brief Visit all reachable detector volumes of the detector
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be handed to each root volume,
  /// eventually contained volumes within the root volumes are
  /// handled by the root volume
  ///
  /// @note if a context is needed for the visit, the vistitor has to provide
  /// it, e.g. as a private member
  template <DetectorVolumeVisitor visitor_t>
  void visitVolumes(visitor_t&& visitor) const {
    visitor(this);
    for (const auto& v : volumes()) {
      v->visitVolumes(std::forward<visitor_t>(visitor));
    }
  }

  /// @brief Visit all reachable detector volumes of the detector - non-const
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be handed to each root volume,
  /// eventually contained volumes within the root volumes are
  /// handled by the root volume
  ///
  /// @note if a context is needed for the visit, the vistitor has to provide
  /// it, e.g. as a private member
  template <MutableDetectorVolumeVisitor visitor_t>
  void visitMutableVolumes(visitor_t&& visitor) {
    visitor(this);
    for (auto& v : volumePtrs()) {
      v->visitMutableVolumes(std::forward<visitor_t>(visitor));
    }
  }

  /// This method allows to udate the navigation state updator
  /// module.
  ///
  /// @param internalNavigation the new navigation state updator for surfaces
  /// @param surfaces the surfaces the new navigation state updator points to
  /// @param volumes the volumes the new navigation state updator points to
  ///
  void assignInternalNavigation(
      InternalNavigationDelegate internalNavigation,
      const std::vector<std::shared_ptr<Surface>>& surfaces = {},
      const std::vector<std::shared_ptr<DetectorVolume>>& volumes = {});

  /// Const access to the navigation state updator
  const InternalNavigationDelegate& internalNavigation() const;

  /// Update a portal given a portal index
  ///
  /// @param portal the portal to be updated
  /// @param pIndex the portal index
  ///
  /// @note throws exception if portal index out of bounds
  void updatePortal(std::shared_ptr<Portal> portal,
                    unsigned int pIndex) noexcept(false);

  /// Final closing of portal, i.e. this sets the end of world
  void closePortals();

  /// Assign the volume material description
  ///
  /// This method allows to load a material description during the
  /// detector geometry building, and assigning it (potentially multiple)
  /// times to detector volumes.
  ///
  /// @param material Material description associated to this volumw
  void assignVolumeMaterial(std::shared_ptr<const IVolumeMaterial> material);

  /// Const access to the volume amterial
  const IVolumeMaterial* volumeMaterial() const;

  /// @return the name of the volume
  const std::string& name() const;

  /// @return the geometry identifier
  const GeometryIdentifier& geometryId() const;

  /// Set the geometry identifier
  /// @note no checking is done, it will overwrite any existing id
  ///
  /// @param geoID is the geometry Id that is set to the object
  void assignGeometryId(const GeometryIdentifier& geoID);

  /// Assign Detector to this volume (for back navigation issues)
  /// @param detector the parenting detector class
  void assignDetector(const Detector& detector);

  /// Const access to the detector
  const Detector* detector() const;

  const BoundingBox& getBoundingBox() const;

 private:
  /// Internal construction method that calls the portal generator
  ///
  /// @param gctx the current geometry context object, e.g. alignment
  /// @param portalGenerator the generator for portals
  ///
  /// @note throws exception if provided parameters are inconsistent
  void construct(const GeometryContext& gctx,
                 const PortalGenerator& portalGenerator) noexcept(false);

  // Check containment - only in debug mode
  ///
  /// @param gctx the current geometry context object, e.g. alignment
  /// @param nseg is the number of segments to approximate
  ///
  /// @return a boolean indicating if the objects are properly contained
  bool checkContainment(const GeometryContext& gctx,
                        std::size_t nseg = 1) const;

  /// build the bounding box
  ///
  void createBoundingBox(const GeometryContext& gctx);

  /// Name of the volume
  std::string m_name = "Unnamed";

  /// Transform to place the bolume
  Transform3 m_transform = Transform3::Identity();

  /// Volume boundaries
  std::shared_ptr<VolumeBounds> m_bounds = nullptr;

  /// Portal store (internal/external)
  ObjectStore<std::shared_ptr<Portal>> m_portals;

  /// Surface store (internal/external)
  ObjectStore<std::shared_ptr<Surface>> m_surfaces;

  /// Volume store (internal/external)
  ObjectStore<std::shared_ptr<DetectorVolume>> m_volumes;

  /// BoundingBox
  std::shared_ptr<const BoundingBox> m_boundingBox;

  ExternalNavigationDelegate m_externalNavigation;

  /// The navigation state updator
  InternalNavigationDelegate m_internalNavigation;

  /// Volume material (optional)
  std::shared_ptr<const IVolumeMaterial> m_volumeMaterial = nullptr;

  /// GeometryIdentifier of this volume
  GeometryIdentifier m_geometryId{0};

  /// The detector it belongs to
  const Detector* m_detector = nullptr;
};

/// @brief  A detector volume factory which first constructs the detector volume
/// and then constructs the portals. This ensures that the std::shared_ptr
/// holding the detector volume is not weak when assigning to the portals.
///
/// @note Optional containment check is invoked by setting the number
/// of segments nSeg to be greater than 0
class DetectorVolumeFactory {
 public:
  /// Create a detector volume - from factory
  static std::shared_ptr<DetectorVolume> construct(
      const PortalGenerator& portalGenerator, const GeometryContext& gctx,
      const std::string& name, const Transform3& transform,
      std::shared_ptr<VolumeBounds> bounds,
      const std::vector<std::shared_ptr<Surface>>& surfaces,
      const std::vector<std::shared_ptr<DetectorVolume>>& volumes,
      ExternalNavigationDelegate externalNavigation,
      InternalNavigationDelegate internalNavigation, int nSeg = -1) {
    auto dVolume = DetectorVolume::makeShared(
        gctx, name, transform, std::move(bounds), surfaces, volumes,
        std::move(externalNavigation), std::move(internalNavigation));
    dVolume->construct(gctx, portalGenerator);

    /// Volume extent is constructed from the portals
    /// So the surface/subvolume containment
    /// check has to happen here
    if (nSeg > 0 && !dVolume->checkContainment(gctx, nSeg)) {
      throw std::invalid_argument(
          "DetectorVolume: surfaces or subvolumes are not contained by volume");
    }
    return dVolume;
  }

  /// Create a detector volume - from factory
  static std::shared_ptr<DetectorVolume> construct(
      const PortalGenerator& portalGenerator, const GeometryContext& gctx,
      std::string name, const Transform3& transform,
      std::shared_ptr<VolumeBounds> bounds,
      InternalNavigationDelegate internalNavigation) {
    auto dVolume = DetectorVolume::makeShared(gctx, std::move(name), transform,
                                              std::move(bounds),
                                              std::move(internalNavigation));
    dVolume->construct(gctx, portalGenerator);
    return dVolume;
  }
};

/// Helper extractors: all portals
struct AllPortalsExtractor {
  /// Extract the portals from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  ///
  /// @return a vector of raw Portal pointers
  inline static const std::vector<const Portal*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState) {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "AllPortalsExtractor: no detector volume given.");
    }
    return nState.currentVolume->portals();
  }
};

/// Helper extractors: all surfaces
struct AllSurfacesExtractor {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  /// @param indices is an ignored index vector
  ///
  /// @return a vector of raw Surface pointers
  inline static const std::vector<const Surface*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState,
      [[maybe_unused]] const std::vector<std::size_t>& indices = {}) {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "AllSurfacesExtractor: no detector volume given.");
    }
    return nState.currentVolume->surfaces();
  }
};

/// Helper extractors: indexed surfaces
struct IndexedSurfacesExtractor {
  /// Extract the surfaces from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  /// @param indices are access indices into the surfaces store
  ///
  /// @note no out of boudns checking is done
  ///
  /// @return a vector of raw Surface pointers
  inline static const std::vector<const Surface*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState, const std::vector<std::size_t>& indices) {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "IndexedSurfacesExtractor: no detector volume given.");
    }
    // Get the surface container
    const auto& surfaces = nState.currentVolume->surfaces();
    // The extracted surfaces
    std::vector<const Surface*> eSurfaces;
    eSurfaces.reserve(indices.size());
    std::for_each(indices.begin(), indices.end(),
                  [&](const auto& i) { eSurfaces.push_back(surfaces[i]); });
    return eSurfaces;
  }
};

/// Helper extractors: all sub volumes of a volume
struct AllSubVolumesExtractor {
  /// Extract the sub volumes from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  /// @param indices are access indices into the volume store (ignored)
  ///
  /// @return a vector of raw DetectorVolume pointers
  inline static const std::vector<const DetectorVolume*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState,
      [[maybe_unused]] const std::vector<std::size_t>& indices = {}) {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "AllSubVolumesExtractor: no detector volume given.");
    }
    return nState.currentVolume->volumes();
  }
};

/// Helper extractors: indexed sub volume of a volume
struct IndexedSubVolumesExtractor {
  /// Extract the sub volumes from the volume
  ///
  /// @param gctx the geometry contextfor this extraction call
  /// @param nState is the current navigation state
  /// @param indices are access indices into the volume store
  ///
  /// @return a vector of raw DetectorVolume pointers
  inline static const std::vector<const DetectorVolume*> extract(
      [[maybe_unused]] const GeometryContext& gctx,
      const NavigationState& nState, const std::vector<std::size_t>& indices) {
    if (nState.currentVolume == nullptr) {
      throw std::runtime_error(
          "AllSubVolumesExtractor: no detector volume given.");
    }
    // Get the sub volumes container
    const auto& volumes = nState.currentVolume->volumes();
    // The extracted volumes
    std::vector<const DetectorVolume*> eVolumes;
    eVolumes.reserve(indices.size());
    std::for_each(indices.begin(), indices.end(),
                  [&](const auto& i) { eVolumes.push_back(volumes[i]); });
    return eVolumes;
  }
};

}  // namespace Experimental
}  // namespace Acts
