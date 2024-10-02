// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Material/Material.hpp"

#include <iosfwd>
#include <utility>
#include <vector>

namespace Acts {

/// Material description for an object with defined thickness.
///
/// This is intended to describe concrete surface materials.
///
/// @see Material for a description of the available parameters.
class MaterialSlab {
 public:
  /// Combine material properties of two layers by averaging them.
  ///
  /// @param layerA Input layer A to average over.
  /// @param layerB Input layer B to average over.
  ///
  /// @return The resulting object has the combined thickness of all layers but just
  ///         one set of appropriately averaged material constants.
  static MaterialSlab averageLayers(const MaterialSlab& layerA,
                                    const MaterialSlab& layerB);

  /// Combine material properties of multiple layers by averaging them.
  ///
  /// @param layers Input layers to average over.
  ///
  /// @return The resulting object has the combined thickness of all layers but just
  ///         one set of appropriately averaged material constants.
  static MaterialSlab averageLayers(const std::vector<MaterialSlab>& layers);

  /// Construct vacuum without thickness.
  MaterialSlab() = default;
  /// Construct vacuum with thickness.
  explicit MaterialSlab(float thickness);
  /// Construct from material description.
  ///
  /// @param material  is the material description
  /// @param thickness is the thickness of the material
  MaterialSlab(const Material& material, float thickness);
  ~MaterialSlab() = default;

  MaterialSlab(MaterialSlab&&) = default;
  MaterialSlab(const MaterialSlab&) = default;
  MaterialSlab& operator=(MaterialSlab&&) = default;
  MaterialSlab& operator=(const MaterialSlab&) = default;

  /// Scale the material thickness by the given factor.
  void scaleThickness(float scale);

  /// Check if the material is valid, i.e. it is finite and not vacuum.
  bool isValid() const { return m_material.isValid() && (0.0f < m_thickness); }

  /// Access the (average) material parameters.
  constexpr const Material& material() const { return m_material; }
  /// Return the thickness.
  constexpr float thickness() const { return m_thickness; }
  /// Return the radiation length fraction.
  constexpr float thicknessInX0() const { return m_thicknessInX0; }
  /// Return the nuclear interaction length fraction.
  constexpr float thicknessInL0() const { return m_thicknessInL0; }

 private:
  Material m_material;
  float m_thickness = 0.0f;
  float m_thicknessInX0 = 0.0f;
  float m_thicknessInL0 = 0.0f;

  friend constexpr bool operator==(const MaterialSlab& lhs,
                                   const MaterialSlab& rhs) {
    // t/X0 and t/L0 are dependent variables and need not be checked
    return (lhs.m_material == rhs.m_material) &&
           (lhs.m_thickness == rhs.m_thickness);
  }
};

std::ostream& operator<<(std::ostream& os, const MaterialSlab& materialSlab);

// Useful typedefs
using MaterialSlabVector = std::vector<MaterialSlab>;
using MaterialSlabMatrix = std::vector<MaterialSlabVector>;

/// list of point used in the mapping of a volume
using RecordedMaterialVolumePoint =
    std::vector<std::pair<Acts::MaterialSlab, std::vector<Acts::Vector3>>>;

}  // namespace Acts
