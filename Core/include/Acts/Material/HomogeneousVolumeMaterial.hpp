// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/Material.hpp"

#include <iosfwd>

namespace Acts {

/// @class HomogeneousVolumeMaterial
///
/// It extends the IVolumeMaterial base class to describe a simple
/// homogeneous material in a volume
class HomogeneousVolumeMaterial : public IVolumeMaterial {
 public:
  /// Explicit constructor
  ///
  /// @param material is the material held by this
  HomogeneousVolumeMaterial(const Material& material);

  /// Copy Constructor
  ///
  /// @param hvm is the source material
  HomogeneousVolumeMaterial(const HomogeneousVolumeMaterial& hvm) = default;

  /// Copy Move Constructor
  ///
  /// @param hvm is the source material
  HomogeneousVolumeMaterial(HomogeneousVolumeMaterial&& hvm) = default;

  /// Destructor
  ~HomogeneousVolumeMaterial() override = default;

  /// Assignment operator
  ///
  /// @param hvm is the source material
  HomogeneousVolumeMaterial& operator=(const HomogeneousVolumeMaterial& hvm) =
      default;

  /// Access to actual material
  ///
  /// @param position is the request position for the material call
  /// @note @p position is ignored
  /// @todo interface to change including 'cell'
  const Material material(const Vector3& position) const final;

  /// Output Method for std::ostream
  ///
  /// @param sl The outoput stream
  std::ostream& toStream(std::ostream& sl) const final;

 private:
  Material m_material;

  /// @brief Check if two materials are exactly equal.
  ///
  /// This is a strict equality check, i.e. the materials must have identical
  /// properties.
  ///
  /// @param lhs is the left hand side material
  /// @param rhs is the right hand side material
  ///
  /// @return true if the materials are equal
  friend constexpr bool operator==(const HomogeneousVolumeMaterial& lhs,
                                   const HomogeneousVolumeMaterial& rhs) {
    return lhs.m_material == rhs.m_material;
  }
};

}  // namespace Acts
