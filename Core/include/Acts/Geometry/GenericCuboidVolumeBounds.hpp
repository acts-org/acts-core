// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Geometry/VolumeBounds.hpp"

#include <array>
#include <cstddef>
#include <ostream>
#include <vector>

namespace Acts {

class IVisualization3D;

class GenericCuboidVolumeBounds : public VolumeBounds {
 public:
  /// @brief  This struct helps to symmetrize with the
  /// the other volume bounds classes
  struct BoundValues {
    static constexpr std::size_t eSize = 24;
  };

  GenericCuboidVolumeBounds() = delete;

  /// Constructor from a set of vertices
  ///
  /// @param vertices The set of input vertices
  ///
  /// The ordering is considered to be:
  /// - the first 4 vertices are the "top" face
  /// - the second 4 vertices are the "bottom" face
  /// - both faces are given in counter clock wise order
  GenericCuboidVolumeBounds(
      const std::array<Acts::Vector3, 8>& vertices) noexcept(false);

  /// Constructor from a fixed size array
  ///
  /// @param values The input values
  GenericCuboidVolumeBounds(
      const std::array<double, BoundValues::eSize>& values) noexcept(false);

  ~GenericCuboidVolumeBounds() override = default;

  VolumeBounds::BoundsType type() const final {
    return VolumeBounds::eGenericCuboid;
  }

  /// Return the bound values as dynamically sized vector
  ///
  /// @return this returns a copy of the internal values
  std::vector<double> values() const final;

  /// Checking if position given in volume frame is inside
  ///
  /// @param gpos is the global position to be checked
  /// @param tol is the tolerance applied for the inside check
  ///
  /// @return boolean indicating if the position is inside
  bool inside(const Vector3& gpos, double tol = 0.) const override;

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
  std::vector<OrientedSurface> orientedSurfaces(
      const Transform3& transform = Transform3::Identity()) const override;

  /// Construct bounding box for this shape
  /// @param trf Optional transform
  /// @param envelope Optional envelope to add / subtract from min/max
  /// @param entity Entity to associate this bounding box with
  /// @return Constructed bounding box
  Volume::BoundingBox boundingBox(const Transform3* trf = nullptr,
                                  const Vector3& envelope = {0, 0, 0},
                                  const Volume* entity = nullptr) const final;

  /// Get the canonical binning values, i.e. the binning values
  /// for that fully describe the shape's extent
  ///
  /// @return vector of canonical binning values
  std::vector<Acts::BinningValue> canonicalBinning() const override {
    return {Acts::BinningValue::binX, Acts::BinningValue::binY,
            Acts::BinningValue::binZ};
  };

  /// @param sl is the output stream to be written into
  std::ostream& toStream(std::ostream& sl) const override;

  /// Draw this shape using a visualization helper
  /// @param helper The visualizatin helper
  /// @param transform Optional transformation matrix
  ///
  void draw(IVisualization3D& helper,
            const Transform3& transform = Transform3::Identity()) const;

 private:
  std::array<Vector3, 8> m_vertices;
  std::array<Vector3, 6> m_normals;

  /// Private helper method to construct the Volume bounds
  /// to be called by the constructors, from the ordered input vertices
  void construct() noexcept(false);
};

}  // namespace Acts
