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
#include "Acts/Geometry/BoundarySurfaceFace.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinnedArray.hpp"

#include <memory>

namespace Acts {

/// @class BoundarySurfaceT
///
/// @tparam  volume_t the type of volume.
///
/// The boundary surface class combines a Surface with the information of a
/// volume.
/// It's templated in the type of volume in order to allow for a return type tat
/// is usable in the navigation stream.
///
/// @note along/oppose definitions are given with respect to the normal vector
/// of the boundary surface.
///
/// @todo change to one schema with BinnedArray0D

template <class volume_t>
class BoundarySurfaceT {
#ifndef DOXYGEN
  friend volume_t;
#endif

  using VolumePtr = std::shared_ptr<const volume_t>;
  using VolumeArray = BinnedArray<VolumePtr>;

 public:
  BoundarySurfaceT()
      : m_surface(nullptr),
        m_oppositeVolume(nullptr),
        m_alongVolume(nullptr),
        m_oppositeVolumeArray(nullptr),
        m_alongVolumeArray(nullptr) {}

  /// Constructor for a Boundary with exact two Volumes attached to it
  /// - usually used in a volume constructor
  ///
  /// @param surface The unique surface the boundary represents
  /// @param inside The inside volume the boundary surface points to
  /// @param outside The outside volume the boundary surface points to
  BoundarySurfaceT(std::shared_ptr<const RegularSurface> surface,
                   const volume_t* inside, const volume_t* outside)
      : m_surface(std::move(surface)),
        m_oppositeVolume(inside),
        m_alongVolume(outside),
        m_oppositeVolumeArray(nullptr),
        m_alongVolumeArray(nullptr) {}

  /// Constructor for a Boundary with exact two Volumes attached to it
  /// - usually used in a volume constructor
  ///
  /// @param surface The unique surface the boundary represents
  /// @param inside The inside volume the boundary surface points to
  /// @param outside The outside volume the boundary surface points to
  BoundarySurfaceT(std::shared_ptr<const RegularSurface> surface,
                   VolumePtr inside, VolumePtr outside)
      : m_surface(std::move(surface)),
        m_oppositeVolume(inside.get()),
        m_alongVolume(outside.get()),
        m_oppositeVolumeArray(nullptr),
        m_alongVolumeArray(nullptr) {}

  /// Constructor for a Boundary with exact multiple Volumes attached to it
  /// - usually used in a volume constructor
  ///
  /// @param surface The unique surface the boundary represents
  /// @param insideArray The inside volume array the boundary surface points to
  /// @param outsideArray The outside volume array the boundary surface
  /// points to
  BoundarySurfaceT(std::shared_ptr<const RegularSurface> surface,
                   std::shared_ptr<const VolumeArray> insideArray,
                   std::shared_ptr<const VolumeArray> outsideArray)
      : m_surface(std::move(surface)),
        m_oppositeVolume(nullptr),
        m_alongVolume(nullptr),
        m_oppositeVolumeArray(insideArray),
        m_alongVolumeArray(outsideArray) {}

  virtual ~BoundarySurfaceT() = default;

  /// Get the next Volume depending on GlobalPosition, GlobalMomentum, dir on
  /// the TrackParameters and the requested direction
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param pos The global position on surface
  /// @param dir The direction on the surface
  ///
  /// @return The attached volume at that position
  virtual const volume_t* attachedVolume(const GeometryContext& gctx,
                                         const Vector3& pos,
                                         const Vector3& dir) const;

  /// templated onBoundary method
  ///
  /// @tparam parameters_t are the parameters to be checked
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param pars The parameters used for this call
  template <class parameters_t>
  bool onBoundary(const GeometryContext& gctx, const parameters_t& pars) const {
    return surfaceRepresentation().isOnSurface(gctx, pars);
  }

  /// The Surface Representation of this
  virtual const RegularSurface& surfaceRepresentation() const;

  /// Helper method: attach a Volume to this BoundarySurfaceT
  /// this is done during the geometry construction.
  ///
  /// @param volume The volume to be attached
  /// @param dir The direction for attaching
  void attachVolume(const volume_t* volume, Direction dir);

  /// Helper method: attach a Volume to this BoundarySurfaceT
  /// this is done during the geometry construction.
  ///
  /// @param volumes The volume array to be attached
  /// @param dir The direction for attaching
  void attachVolumeArray(std::shared_ptr<const VolumeArray> volumes,
                         Direction dir);

 protected:
  /// the represented surface by this
  std::shared_ptr<const RegularSurface> m_surface;
  /// the inside (w.r.t. normal vector) volume to point to if only one exists
  const volume_t* m_oppositeVolume;
  /// the outside (w.r.t. normal vector) volume to point to if only one exists
  const volume_t* m_alongVolume;
  /// the inside (w.r.t. normal vector) volume array to point to
  std::shared_ptr<const VolumeArray> m_oppositeVolumeArray;
  /// the outside (w.r.t. normal vector) volume array to point to
  std::shared_ptr<const VolumeArray> m_alongVolumeArray;
};

template <class volume_t>
inline const RegularSurface& BoundarySurfaceT<volume_t>::surfaceRepresentation()
    const {
  return (*(m_surface.get()));
}

template <class volume_t>
void BoundarySurfaceT<volume_t>::attachVolume(const volume_t* volume,
                                              Direction dir) {
  if (dir == Direction::Backward) {
    m_oppositeVolume = volume;
  } else {
    m_alongVolume = volume;
  }
}

template <class volume_t>
void BoundarySurfaceT<volume_t>::attachVolumeArray(
    const std::shared_ptr<const VolumeArray> volumes, Direction dir) {
  if (dir == Direction::Backward) {
    m_oppositeVolumeArray = volumes;
  } else {
    m_alongVolumeArray = volumes;
  }
}

template <class volume_t>
const volume_t* BoundarySurfaceT<volume_t>::attachedVolume(
    const GeometryContext& gctx, const Vector3& pos, const Vector3& dir) const {
  const volume_t* attVolume = nullptr;
  // dot product with normal vector to distinguish inside/outside
  if ((surfaceRepresentation().normal(gctx, pos)).dot(dir) > 0.) {
    attVolume = m_alongVolumeArray ? m_alongVolumeArray->object(pos).get()
                                   : m_alongVolume;
  } else {
    attVolume = m_oppositeVolumeArray ? m_oppositeVolumeArray->object(pos).get()
                                      : m_oppositeVolume;
  }
  return attVolume;
}

}  // namespace Acts
