// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <iosfwd>
#include <limits>

#include <Eigen/Dense>

namespace Acts {

/// Material description for interactions with matter.
/// @defgroup Material Material
///
/// The following parameters are used to specify the material and its
/// interactions with traversing particles:
///
/// - radiation length X0 (native length units)
/// - nuclear interaction length L0 (native length units)
/// - relative atomic mass Ar (unitless number)
/// - nuclear charge number Z (elementary charge e)
/// - molar density (native amount-of-substance unit / (native length unit)³)
///
/// The parameters can be effective or average parameters e.g. when a mixture
/// of materials is described.
///
/// @note Always use the opaque parameters vector to serialize/deserialize the
///   material information. Since the internal storage might be different from
///   the external accessors, this ensures that always the numerically optimal
///   parameters are stored. Use the `ParametersVector` type and do not assume
///   any particular size since we might consider to store more parameters in
///   the future.
class Material {
 public:
  using ParametersVector = Eigen::Matrix<float, 5, 1>;

  static constexpr Material Vacuum() { return Material(); }

  // Both mass and molar density are stored as a float and can thus not be
  // distinguished by their types. Just changing the last element in the
  // previously existing constructor that took five floats as input to represent
  // molar density instead of mass density could have lead to significant
  // confusion compared to the previous behaviour. To avoid any ambiguity,
  // construction from separate material parameters must happen through the
  // following named constructors.

  /// Construct from material parameters using the molar density.
  ///
  /// @param x0 is the radiation length
  /// @param l0 is the nuclear interaction length
  /// @param ar is the relative atomic mass
  /// @param z is the nuclear charge number
  /// @param molarRho is the molar density
  static Material fromMolarDensity(float x0, float l0, float ar, float z,
                                   float molarRho);

  /// Construct from material parameters using the mass density.
  ///
  /// @param x0 is the radiation length
  /// @param l0 is the nuclear interaction length
  /// @param ar is the relative atomic mass
  /// @param z is the nuclear charge number
  /// @param massRho is the mass density
  ///
  /// @warning Due to the choice of native mass units, using the mass density
  ///   can lead to numerical problems. Typical mass densities lead to
  ///   computations with values differing by 20+ orders of magnitude.
  static Material fromMassDensity(float x0, float l0, float ar, float z,
                                  float massRho);

  /// Construct from an encoded parameters vector.
  explicit Material(const ParametersVector& parameters);

  /// Check if the material is vacuum.
  bool isVacuum() const { return m_ar <= 0.f; }

  /// Return the radiation length. Infinity in case of vacuum.
  constexpr float X0() const { return m_x0; }
  /// Return the nuclear interaction length. Infinity in case of vacuum.
  constexpr float L0() const { return m_l0; }
  /// Return the relative atomic mass.
  constexpr float Ar() const { return m_ar; }
  /// Return the nuclear charge number.
  constexpr float Z() const { return m_z; }
  /// Return the molar density.
  constexpr float molarDensity() const { return m_molarRho; }
  /// Return the molar electron density.
  constexpr float molarElectronDensity() const { return m_z * m_molarRho; }
  /// Return the mass density.
  float massDensity() const;
  /// Return the mean electron excitation energy.
  float meanExcitationEnergy() const;

  /// Encode the properties into an opaque parameters vector.
  ParametersVector parameters() const;

 private:
  float m_x0 = std::numeric_limits<float>::infinity();
  float m_l0 = std::numeric_limits<float>::infinity();
  float m_ar = 0.0f;
  float m_z = 0.0f;
  float m_molarRho = 0.0f;

  constexpr Material() = default;

  /// @brief Check if two materials are exactly equal.
  ///
  /// This is a strict equality check, i.e. the materials must have identical
  /// properties.
  ///
  /// @param lhs is the left hand side material
  /// @param rhs is the right hand side material
  ///
  /// @return true if the materials are equal
  friend constexpr bool operator==(const Material& lhs, const Material& rhs) {
    return (lhs.m_x0 == rhs.m_x0) && (lhs.m_l0 == rhs.m_l0) &&
           (lhs.m_ar == rhs.m_ar) && (lhs.m_z == rhs.m_z) &&
           (lhs.m_molarRho == rhs.m_molarRho);
  }
};

std::ostream& operator<<(std::ostream& os, const Material& material);

}  // namespace Acts
