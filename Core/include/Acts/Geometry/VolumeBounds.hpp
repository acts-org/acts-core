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
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace Acts {

class Surface;
class VolumeBounds;
class Direction;

struct OrientedSurface {
  std::shared_ptr<RegularSurface> surface;
  Direction direction;
};

// Planar definitions to help construct the boundary surfaces
static const Transform3 s_planeXY = Transform3::Identity();
static const Transform3 s_planeYZ = AngleAxis3(0.5 * M_PI, Vector3::UnitY()) *
                                    AngleAxis3(0.5 * M_PI, Vector3::UnitZ()) *
                                    Transform3::Identity();
static const Transform3 s_planeZX = AngleAxis3(-0.5 * M_PI, Vector3::UnitX()) *
                                    AngleAxis3(-0.5 * M_PI, Vector3::UnitZ()) *
                                    Transform3::Identity();

/// @class VolumeBounds
///
/// Pure Absract Base Class for Volume bounds.
///
/// Acts::VolumeBounds are a set of up to six confining Surfaces that are stored
/// in a std::vector.
/// Each type of Acts::VolumeBounds has to implement a orientedSurfaces() and
/// a inside() method.
///
/// The Volume, retrieving a set of Surfaces from the VolumeBounds, can turn the
/// Surfaces into BoundarySurfaces.

class VolumeBounds {
 public:
  // @enum BoundsType
  /// This is nested to the VolumeBounds, as also SurfaceBounds will have
  /// Bounds Type.
  enum class BoundsType {
    eCone,
    eCuboid,
    eCutoutCylinder,
    eCylinder,
    eGenericCuboid,
    eTrapezoid,
    eOther,

  };

  using enum BoundsType;

  /// Static member to get the name of the BoundsType
  static const std::vector<std::string> s_boundsTypeNames;

  VolumeBounds() = default;

  virtual ~VolumeBounds() = default;

  /// Return the bounds type - for persistency optimization
  ///
  /// @return is a BoundsType enum
  virtual BoundsType type() const = 0;

  /// Access method for bound values, this is a dynamically sized
  /// vector containing the parameters needed to describe these bounds
  ///
  /// @return of the stored values for this SurfaceBounds object
  virtual std::vector<double> values() const = 0;

  /// Checking if position given in volume frame is inside
  ///
  /// @param gpos is the global position to be checked
  /// @param tol is the tolerance applied for the inside check
  ///
  /// @return boolean indicating if the position is inside
  virtual bool inside(const Vector3& gpos, double tol = 0.) const = 0;

  /// Oriented surfaces, i.e. the decomposed boundary surfaces and the
  /// according navigation direction into the volume given the normal
  /// vector on the surface
  ///
  /// @param transform is the 3D transform to be applied to the boundary
  /// surfaces to position them in 3D space
  ///
  /// It will throw an exception if the orientation prescription is not adequate
  ///
  /// @return a vector of surfaces bounding this volume
  virtual std::vector<OrientedSurface> orientedSurfaces(
      const Transform3& transform = Transform3::Identity()) const = 0;

  /// Construct bounding box for this shape
  /// @param trf Optional transform
  /// @param envelope Optional envelope to add / subtract from min/max
  /// @param entity Entity to associate this bounding box with
  /// @return Constructed bounding box
  virtual Volume::BoundingBox boundingBox(
      const Transform3* trf = nullptr, const Vector3& envelope = {0, 0, 0},
      const Volume* entity = nullptr) const = 0;

  /// Get the canonical binning values, i.e. the binning values
  /// for that fully describe the shape's extent
  ///
  /// @return vector of canonical binning values
  ///
  /// @note This is the default implementation that
  /// returns the bounding box binning. Individual shapes
  /// should override this method
  virtual std::vector<Acts::BinningValue> canonicalBinning() const {
    return {Acts::BinningValue::binX, Acts::BinningValue::binY,
            Acts::BinningValue::binZ};
  };

  /// Binning offset - overloaded for some R-binning types
  ///
  /// @param bValue is the binning schema used
  ///
  /// @return vector 3D to be used for the binning
  virtual Vector3 binningOffset(BinningValue bValue) const;

  /// Binning borders in double
  ///
  /// @param bValue is the binning schema used
  ///
  /// @return float offset to be used for the binning
  virtual double binningBorder(BinningValue bValue) const;

  /// Output Method for std::ostream, to be overloaded by child classes
  ///
  /// @param sl is the output stream to be dumped into
  virtual std::ostream& toStream(std::ostream& sl) const = 0;
};

/// Binning offset - overloaded for some R-binning types
inline Vector3 VolumeBounds::binningOffset(
    BinningValue /*bValue*/) const {  // standard offset is 0.,0.,0.
  return Vector3(0., 0., 0.);
}

inline double VolumeBounds::binningBorder(BinningValue /*bValue*/) const {
  return 0.;
}

/// Overload of << operator for std::ostream for debug output
std::ostream& operator<<(std::ostream& sl, const VolumeBounds& vb);

inline bool operator==(const VolumeBounds& lhs, const VolumeBounds& rhs) {
  if (&lhs == &rhs) {
    return true;
  }
  return (lhs.type() == rhs.type()) && (lhs.values() == rhs.values());
}

std::ostream& operator<<(std::ostream& sl, const VolumeBounds::BoundsType& bt);

}  // namespace Acts
