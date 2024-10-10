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
#include "Acts/Geometry/GeometryObject.hpp"
#include "Acts/Utilities/BinningType.hpp"
#include "Acts/Utilities/BoundingBox.hpp"

#include <iosfwd>
#include <memory>
#include <optional>

namespace Acts {

class VolumeBounds;

/// @class Volume
///
/// It inherits from GeometryObject for geometry identification
///
/// Base class for all volumes inside the tracking realm, it defines the
/// interface for inherited Volume classes regarding the geometrical
/// information.
class Volume : public GeometryObject {
 public:
  using BoundingBox = AxisAlignedBoundingBox<Volume, ActsScalar, 3>;

  /// Explicit constructor with shared arguments
  ///
  /// @param transform is the transform to position the volume in 3D space
  /// @param volbounds is the volume boundary definitions
  Volume(const Transform3& transform, std::shared_ptr<VolumeBounds> volbounds);

  /// Copy Constructor - with optional shift
  ///
  /// @param vol is the source volume for the copy
  /// @param shift is the optional shift applied as : shift * vol.transform()
  Volume(const Volume& vol, const Transform3& shift = Transform3::Identity());

  Volume() = delete;
  virtual ~Volume() = default;

  /// Assignment operator
  ///
  /// @param vol is the source volume to be copied
  Volume& operator=(const Volume& vol);

  /// Return methods for geometry transform
  const Transform3& transform() const;

  /// Returns the inverted transform of this volume.
  const Transform3& itransform() const;

  void setTransform(const Transform3& transform);

  /// returns the center of the volume
  const Vector3& center() const;

  /// Returns a const reference to the volume bounds
  const VolumeBounds& volumeBounds() const;

  /// Returns a mutable reference to the volume bounds
  VolumeBounds& volumeBounds();

  /// Returns shared pointer to the volume bounds
  std::shared_ptr<const VolumeBounds> volumeBoundsPtr() const;

  /// Returns shared pointer to the volume bounds
  std::shared_ptr<VolumeBounds> volumeBoundsPtr();

  /// Set volume bounds and update volume bounding boxes implicitly
  /// @param volbounds The volume bounds to be assigned
  void assignVolumeBounds(std::shared_ptr<VolumeBounds> volbounds);

  /// Set the volume bounds and optionally also update the volume transform
  /// @param volbounds The volume bounds to be assigned
  /// @param transform The transform to be assigned, can be optional
  virtual void update(std::shared_ptr<VolumeBounds> volbounds,
                      std::optional<Transform3> transform = std::nullopt);

  /// Construct bounding box for this shape
  /// @param envelope Optional envelope to add / subtract from min/max
  /// @return Constructed bounding box pointing to this volume
  BoundingBox boundingBox(const Vector3& envelope = {0, 0, 0}) const;

  /// Construct oriented bounding box for this shape
  /// @note This will build an oriented bounding box with an
  ///       envelope value of (0.05, 0.05, 0.05)mm
  /// @return Constructed oriented bounding box pointing to this volume
  BoundingBox orientedBoundingBox() const;

  /// Inside() method for checks
  ///
  /// @param gpos is the position to be checked
  /// @param tol is the tolerance parameter
  ///
  /// @return boolean indicator if the position is inside
  bool inside(const Vector3& gpos, ActsScalar tol = 0.) const;

  /// The binning position method
  /// - as default the center is given, but may be overloaded
  ///
  /// @param gctx The current geometry context object, e.g. alignment
  /// @param bValue is the binning value schema
  ///
  /// @return vector 3D that can be used for the binning
  Vector3 binningPosition(const GeometryContext& gctx,
                          BinningValue bValue) const override;

  bool operator==(const Volume& other) const;

  /// Produces a 3D visualization of this volume
  /// @param helper The visualization helper describing the output format
  /// @param gctx The geometry context
  /// @param viewConfig The view configuration
  void visualize(IVisualization3D& helper, const GeometryContext& gctx,
                 const ViewConfig& viewConfig) const;

 protected:
  Transform3 m_transform;
  Transform3 m_itransform;
  Vector3 m_center;

 private:
  std::shared_ptr<VolumeBounds> m_volumeBounds;
};

/**Overload of << operator for std::ostream for debug output*/
std::ostream& operator<<(std::ostream& sl, const Volume& vol);

}  // namespace Acts
