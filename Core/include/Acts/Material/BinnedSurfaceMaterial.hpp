// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Utilities/BinUtility.hpp"

#include <cstddef>
#include <iosfwd>
#include <vector>

namespace Acts {

/// @class BinnedSurfaceMaterial
///
/// It extends the @c ISurfaceMaterial base class and is an array pf
/// MaterialSlab. This is not memory optimised as every bin
/// holds one material property object.

class BinnedSurfaceMaterial : public ISurfaceMaterial {
 public:
  /// Default Constructor - deleted
  BinnedSurfaceMaterial() = delete;

  /// Explicit constructor with only full MaterialSlab,
  /// for one-dimensional binning.
  ///
  /// The split factors:
  ///    - 1. : oppositePre
  ///    - 0. : alongPre
  ///  ===> 1 Dimensional array
  ///
  /// @param binUtility defines the binning structure on the surface (copied)
  /// @param fullProperties is the vector of properties as recorded (moved)
  /// @param splitFactor is the pre/post splitting directive
  /// @param mappingType is the type of surface mapping associated to the surface
  BinnedSurfaceMaterial(const BinUtility& binUtility,
                        MaterialSlabVector fullProperties,
                        double splitFactor = 0.,
                        MappingType mappingType = MappingType::Default);

  /// Explicit constructor with only full MaterialSlab,
  /// for two-dimensional binning.
  ///
  /// The split factors:
  ///    - 1. : oppositePre
  ///    - 0. : alongPre
  ///  ===> 1 Dimensional array
  ///
  /// @param binUtility defines the binning structure on the surface (copied)
  /// @param fullProperties is the vector of properties as recorded (moved)
  /// @param splitFactor is the pre/post splitting directive
  /// @param mappingType is the type of surface mapping associated to the surface
  BinnedSurfaceMaterial(const BinUtility& binUtility,
                        MaterialSlabMatrix fullProperties,
                        double splitFactor = 0.,
                        MappingType mappingType = MappingType::Default);

  /// Copy Move Constructor
  ///
  /// @param bsm is the source object to be copied
  BinnedSurfaceMaterial(BinnedSurfaceMaterial&& bsm) = default;

  /// Copy Constructor
  ///
  /// @param bsm is the source object to be copied
  BinnedSurfaceMaterial(const BinnedSurfaceMaterial& bsm) = default;

  /// Assignment Move operator
  BinnedSurfaceMaterial& operator=(BinnedSurfaceMaterial&& bsm) = default;

  /// Assignment operator
  BinnedSurfaceMaterial& operator=(const BinnedSurfaceMaterial& bsm) = default;

  /// Destructor
  ~BinnedSurfaceMaterial() override = default;

  /// Scale operator
  ///
  /// @param scale is the scale factor for the full material
  BinnedSurfaceMaterial& operator*=(double scale) final;

  /// Return the BinUtility
  const BinUtility& binUtility() const;

  /// @brief Retrieve the entire material slab matrix
  const MaterialSlabMatrix& fullMaterial() const;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector2&) const
  const MaterialSlab& materialSlab(const Vector2& lp) const final;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector3&) const
  const MaterialSlab& materialSlab(const Vector3& gp) const final;

  /// Output Method for std::ostream, to be overloaded by child classes
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  /// The helper for the bin finding
  BinUtility m_binUtility;

  /// The five different MaterialSlab
  MaterialSlabMatrix m_fullMaterial;
};

inline const BinUtility& BinnedSurfaceMaterial::binUtility() const {
  return (m_binUtility);
}

inline const MaterialSlabMatrix& BinnedSurfaceMaterial::fullMaterial() const {
  return m_fullMaterial;
}

}  // namespace Acts
