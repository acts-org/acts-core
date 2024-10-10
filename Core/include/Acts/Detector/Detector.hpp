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
#include "Acts/Detector/DetectorVolume.hpp"
#include "Acts/Detector/DetectorVolumeVisitorConcept.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Surfaces/SurfaceVisitorConcept.hpp"
#include "Acts/Utilities/Delegate.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Acts {

class Surface;

namespace Experimental {
struct NavigationState;

class Detector : public std::enable_shared_from_this<Detector> {
 protected:
  /// Create a detector from volumes
  ///
  /// @param name the detecor name
  /// @param rootVolumes the volumes contained by this detector
  /// @param detectorVolumeFinder is a Delegate to find the associated volume
  ///
  /// @note will throw an exception if volumes vector is empty
  /// @note will throw an exception if duplicate volume names exist
  /// @note will throw an exception if the delegate is not connected
  Detector(std::string name,
           std::vector<std::shared_ptr<DetectorVolume>> rootVolumes,
           ExternalNavigationDelegate detectorVolumeFinder) noexcept(false);

 public:
  /// Factory for producing memory managed instances of Detector.
  static std::shared_ptr<Detector> makeShared(
      std::string name,
      std::vector<std::shared_ptr<DetectorVolume>> rootVolumes,
      ExternalNavigationDelegate detectorVolumeFinder);

  /// Retrieve a @c std::shared_ptr for this surface (non-const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior (but most likely implemented as a @c bad_weak_ptr
  ///       exception), in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<Detector> getSharedPtr();

  /// Retrieve a @c std::shared_ptr for this surface (const version)
  ///
  /// @note Will error if this was not created through the @c makeShared factory
  ///       since it needs access to the original reference. In C++14 this is
  ///       undefined behavior, but most likely implemented as a @c bad_weak_ptr
  ///       exception, in C++17 it is defined as that exception.
  /// @note Only call this if you need shared ownership of this object.
  ///
  /// @return The shared pointer
  std::shared_ptr<const Detector> getSharedPtr() const;

  /// Non-const access to the root volumes
  ///
  /// @return the root volume shared pointer
  std::vector<std::shared_ptr<DetectorVolume>>& rootVolumePtrs();

  /// Const access to the root volumes
  ///
  /// @return a vector to const DetectorVolume raw pointers
  const std::vector<const DetectorVolume*>& rootVolumes() const;

  /// Non-const access to the root volume
  ///
  /// @return the volumes shared pointer store
  std::vector<std::shared_ptr<DetectorVolume>>& volumePtrs();

  /// Const access to sub volumes
  ///
  /// @return a vector to const DetectorVolume raw pointers
  const std::vector<const DetectorVolume*>& volumes() const;

  /// Const access to the hierarchy map of all sensitive surfaces
  ///
  /// @return the map which can be queried with GeometryID for ranges
  const GeometryHierarchyMap<const Surface*>& sensitiveHierarchyMap() const;

  /// Search for a surface with the given identifier.
  ///
  /// @param id is the geometry identifier of the surface
  /// @retval nullptr if no such surface exists
  /// @retval pointer to the found surface otherwise.
  const Surface* findSurface(GeometryIdentifier id) const;

  /// @brief Visit all reachable surfaces of the detector
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be handed to each root volume,
  /// eventually contained volumes within the root volumes are
  /// handled by the root volume
  ///
  /// @note if a context is needed for the visit, the vistitor has to provide
  /// it, e.g. as a private member
  ///
  /// @note due to the fact that portals can be shared between volumes, multiple
  /// visits may occur, duplicated addressing needs to be taken care of by the
  /// visitor
  template <SurfaceVisitor visitor_t>
  void visitSurfaces(visitor_t&& visitor) const {
    for (const auto& v : rootVolumes()) {
      v->template visitSurfaces<visitor_t>(std::forward<visitor_t>(visitor));
    }
  }

  /// @brief Visit all reachable surfaces of the detector - non-const
  ///
  /// @tparam visitor_t Type of the callable visitor
  ///
  /// @param visitor will be handed to each root volume,
  /// eventually contained volumes within the root volumes are
  /// handled by the root volume
  ///
  /// @note if a context is needed for the visit, the vistitor has to provide
  /// it, e.g. as a private member
  ///
  /// @note due to the fact that this doesn't run over root volumes, and
  /// due to the fact that portals can be shared between volumes, multiple
  /// visits may occur, duplicated addressing needs to be taken care of by the
  template <MutableSurfaceVisitor visitor_t>
  void visitMutableSurfaces(visitor_t&& visitor) {
    for (auto& v : volumePtrs()) {
      v->template visitMutableSurfaces<visitor_t>(
          std::forward<visitor_t>(visitor));
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
    for (const auto& v : rootVolumes()) {
      v->template visitVolumes<visitor_t>(std::forward<visitor_t>(visitor));
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
  ///
  /// @note that due to non running over root volumes, multiple visits
  /// may occur, duplicated addressing needs to be taken care of by the
  /// visitor
  template <MutableDetectorVolumeVisitor visitor_t>
  void visitMutableVolumes(visitor_t&& visitor) {
    for (const auto& v : volumePtrs()) {
      v->template visitMutableVolumes<visitor_t>(
          std::forward<visitor_t>(visitor));
    }
  }

  /// Update the current volume of a given navigation state
  ///
  /// @param gctx is the Geometry context of the call
  /// @param nState [in, out] is the navigation state
  ///
  void updateDetectorVolume(const GeometryContext& gctx,
                            NavigationState& nState) const;

  /// Find a volume from a position
  ///
  /// @param gctx is the Geometry context of the call
  /// @param position is the position of the call
  ///
  /// @note this creates internally a NavigationState object
  ///
  /// @return the volume pointer or nullptr (if outside)
  const DetectorVolume* findDetectorVolume(const GeometryContext& gctx,
                                           const Vector3& position) const;

  /// Find a volume by name
  ///
  /// @param name with which the volume is searched for
  ///
  /// @return the volume pointer or nullptr (if not found)
  const DetectorVolume* findDetectorVolume(const std::string& name) const;

  /// Update the volume finder
  ///
  /// @param detectorVolumeFinder the new volume finder
  void updateDetectorVolumeFinder(
      ExternalNavigationDelegate detectorVolumeFinder);

  /// Const access to the volume finder
  const ExternalNavigationDelegate& detectorVolumeFinder() const;

  /// Return the name of the detector
  const std::string& name() const;

 private:
  /// Name of the detector
  std::string m_name;

  /// Root volumes
  DetectorVolume::ObjectStore<std::shared_ptr<DetectorVolume>> m_rootVolumes;

  /// Volume store (internal/external)
  DetectorVolume::ObjectStore<std::shared_ptr<DetectorVolume>> m_volumes;

  /// A volume finder delegate
  ExternalNavigationDelegate m_volumeFinder;

  /// Name/index map to find volumes by name and detect duplicates
  std::unordered_map<std::string, std::size_t> m_volumeNameIndex;

  /// Geometry Id hierarchy map of all sensitive surfaces
  GeometryHierarchyMap<const Surface*> m_sensitiveHierarchyMap;
};

}  // namespace Experimental
}  // namespace Acts
