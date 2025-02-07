// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/MaterialSlab.hpp"

#include <iosfwd>

namespace Acts {

/// @class HomogeneousSurfaceMaterial
///
/// It extends the ISurfaceMaterial virtual base class to describe
/// a simple homogeneous material on a surface
class HomogeneousSurfaceMaterial : public ISurfaceMaterial {
 public:
  /// Default Constructor - defaulted
  HomogeneousSurfaceMaterial() = default;

  /// Explicit constructor
  ///
  /// @param full are the full material properties
  /// @param splitFactor is the split for pre/post update
  /// @param mappingType is the type of surface mapping associated to the surface
  HomogeneousSurfaceMaterial(const MaterialSlab& full, double splitFactor = 1.,
                             MappingType mappingType = MappingType::Default);

  /// Copy Constructor
  ///
  /// @param hsm is the source material
  HomogeneousSurfaceMaterial(const HomogeneousSurfaceMaterial& hsm) = default;

  /// Copy Move Constructor
  ///
  /// @param hsm is the source material
  HomogeneousSurfaceMaterial(HomogeneousSurfaceMaterial&& hsm) = default;

  /// Destructor
  ~HomogeneousSurfaceMaterial() override = default;

  /// Assignment operator
  ///
  /// @param hsm is the source material
  HomogeneousSurfaceMaterial& operator=(const HomogeneousSurfaceMaterial& hsm) =
      default;

  /// Assignment Move operator
  ///
  /// @param hsm is the source material
  HomogeneousSurfaceMaterial& operator=(HomogeneousSurfaceMaterial&& hsm) =
      default;

  /// Scale operator
  /// - it is effectively a thickness scaling
  ///
  /// @param factor is the scale factor
  HomogeneousSurfaceMaterial& scale(double factor) final;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector2&) const
  ///
  /// @note the input parameter is ignored
  const MaterialSlab& materialSlab(const Vector2& lp) const final;

  /// @copydoc ISurfaceMaterial::materialSlab(const Vector3&) const
  ///
  /// @note the input parameter is ignored
  const MaterialSlab& materialSlab(const Vector3& gp = Vector3{0., 0.,
                                                               0.}) const final;

  /// The inherited methods - for MaterialSlab access
  using ISurfaceMaterial::materialSlab;

  /// The inherited methods - for scale access
  using ISurfaceMaterial::factor;

  /// Output Method for std::ostream
  ///
  /// @param sl The outoput stream
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  /// The five different MaterialSlab
  MaterialSlab m_fullMaterial;

  /// @brief Check if two materials are exactly equal.
  ///
  /// This is a strict equality check, i.e. the materials must have identical
  /// properties.
  ///
  /// @param lhs is the left hand side material
  /// @param rhs is the right hand side material
  ///
  /// @return true if the materials are equal
  friend constexpr bool operator==(const HomogeneousSurfaceMaterial& lhs,
                                   const HomogeneousSurfaceMaterial& rhs) {
    return lhs.m_fullMaterial == rhs.m_fullMaterial;
  }
};

}  // namespace Acts
