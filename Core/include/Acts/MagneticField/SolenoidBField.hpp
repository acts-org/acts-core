// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cstddef>
#include <functional>

namespace Acts {

/// @ingroup MagneticField
//
/// @class SolenoidBField
/// Implements a multi-coil solenoid magnetic field. On every call, the field
/// is evaluated at that exact position. The field has radially symmetry, the
/// field vectors point in +z direction.
/// The config exposes a target field value in the center. This value is used
/// to empirically determine a scale factor which reproduces this field value
/// in the center.
///
/// E_1(k^2) = complete elliptic integral of the 1st kind
/// E_2(k^2) = complete elliptic integral of the 2nd kind
///
/// E_1(k^2) and E_2(k^2) are usually indicated as K(k^2) and E(k^2) in
/// literature,
/// respectively
///              _
///     2       /  pi / 2          2    2          - 1 / 2
/// E (k )  =   |         ( 1  -  k  sin {theta} )         dtheta
///  1         _/  0
///
///              _          ____________________
///     2       /  pi / 2| /       2    2
/// E (k )  =   |        |/ 1  -  k  sin {theta} dtheta
///  2         _/  0
///
/// k^2 = is a function of the point (r, z) and of the radius of the coil R
///
///  2           4Rr
/// k   =  ---------------
///               2      2
///        (R + r)   +  z
/// Using these, you can evaluate the two components B_r and B_z of the
/// magnetic field:
///                            _                             _
///              mu  I        |  /     2 \                    |
///                0     kz   |  |2 - k  |    2          2    |
/// B (r, z)  =  ----- ------ |  |-------|E (k )  -  E (k )   |
///  r            4pi     ___ |  |      2| 2          1       |
///                    | /  3 |_ \2 - 2k /                   _|
///                    |/ Rr
///
///                         _                                       _
///             mu  I      |  /         2      \                     |
///               0     k  |  | (R + r)k  - 2r |     2          2    |
/// B (r,z)  =  ----- ---- |  | -------------- | E (k )  +  E (k )   |
///  z           4pi    __ |  |           2    |  2          1       |
///                   |/Rr |_ \   2r(1 - k )   /                    _|
///
class SolenoidBField final : public MagneticFieldProvider {
 public:
  struct Cache {
    /// @brief Constructor with magnetic field context
    Cache(const MagneticFieldContext& /*mctx*/) {}
  };

  /// Config struct for the SolenoidBfield.
  struct Config {
    /// Radius at which the coils are located.
    double radius;
    /// Extent of the solenoid in z. It goes from
    /// -length/2 to +length/2 by convention
    double length;
    /// The number of coils that make up the solenoid
    std::size_t nCoils;
    /// The target magnetic field strength at the center.
    /// This will be used to scale coefficients
    double bMagCenter;
  };

  /// @brief the constructor with a shared pointer
  /// @note since it is a shared field, we enforce it to be const
  /// @tparam bField is the shared BField to be stored
  SolenoidBField(Config config);

  /// @brief Retrieve magnetic field value in local (r,z) coordinates
  ///
  /// @param [in] position local 2D position
  Vector2 getField(const Vector2& position) const;

  /// @copydoc MagneticFieldProvider::makeCache(const MagneticFieldContext&) const
  MagneticFieldProvider::Cache makeCache(
      const MagneticFieldContext& mctx) const override;

  /// @brief Get the B field at a position
  ///
  /// @param position The position to query at
  Vector3 getField(const Vector3& position) const;

  /// @copydoc MagneticFieldProvider::getField(const Vector3&,MagneticFieldProvider::Cache&) const
  Result<Vector3> getField(const Vector3& position,
                           MagneticFieldProvider::Cache& cache) const override;

  /// @copydoc MagneticFieldProvider::getFieldGradient(const Vector3&,ActsMatrix<3,3>&,MagneticFieldProvider::Cache&) const
  ///
  /// @note currently the derivative is not calculated
  /// @todo return derivative
  Result<Vector3> getFieldGradient(
      const Vector3& position, ActsMatrix<3, 3>& derivative,
      MagneticFieldProvider::Cache& cache) const override;

 private:
  Config m_cfg;
  double m_scale;
  double m_dz;
  double m_R2;

  Vector2 multiCoilField(const Vector2& pos, double scale) const;

  Vector2 singleCoilField(const Vector2& pos, double scale) const;

  double B_r(const Vector2& pos, double scale) const;

  double B_z(const Vector2& pos, double scale) const;

  double k2(double r, double z) const;
};

}  // namespace Acts
