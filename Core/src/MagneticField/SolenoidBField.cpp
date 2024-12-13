// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/MagneticField/SolenoidBField.hpp"

#include "Acts/Utilities/VectorHelpers.hpp"

#include <cmath>
#include <numbers>

#define BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS

#include <boost/exception/exception.hpp>
#include <boost/math/special_functions/ellint_1.hpp>
#include <boost/math/special_functions/ellint_2.hpp>

Acts::SolenoidBField::SolenoidBField(Config config) : m_cfg(config) {
  m_dz = m_cfg.length / m_cfg.nCoils;
  m_R2 = m_cfg.radius * m_cfg.radius;
  // we need to scale so we reproduce the expected B field strength
  // at the center of the solenoid
  Vector2 field = multiCoilField({0, 0}, 1.);  // scale = 1
  m_scale = m_cfg.bMagCenter / field.norm();
}

Acts::MagneticFieldProvider::Cache Acts::SolenoidBField::makeCache(
    const MagneticFieldContext& mctx) const {
  return MagneticFieldProvider::Cache(std::in_place_type<Cache>, mctx);
}

Acts::Vector3 Acts::SolenoidBField::getField(const Vector3& position) const {
  using VectorHelpers::perp;
  Vector2 rzPos(perp(position), position.z());
  Vector2 rzField = multiCoilField(rzPos, m_scale);
  Vector3 xyzField(0, 0, rzField[1]);

  if (rzPos[0] != 0.) {
    // add xy field component, radially symmetric
    Vector3 rDir = Vector3(position.x(), position.y(), 0).normalized();
    xyzField += rDir * rzField[0];
  }

  return xyzField;
}

Acts::Vector3 Acts::SolenoidBField::getField(
    const Vector3& position, MagneticFieldProvider::Cache& /*cache*/) const {
  return getField(position);
}

Acts::Vector2 Acts::SolenoidBField::getField(const Vector2& position) const {
  return multiCoilField(position, m_scale);
}

std::pair<Acts::Vector3, Acts::SquareMatrix3>
Acts::SolenoidBField::getFieldAndGradient(
    const Vector3& /*position*/,
    MagneticFieldProvider::Cache& /*cache*/) const {
  throw std::runtime_error("Not implemented");
}

Acts::Vector2 Acts::SolenoidBField::multiCoilField(const Vector2& pos,
                                                   double scale) const {
  // iterate over all coils
  Vector2 resultField(0, 0);
  for (std::size_t coil = 0; coil < m_cfg.nCoils; coil++) {
    Vector2 shiftedPos =
        Vector2(pos[0], pos[1] + m_cfg.length * 0.5 - m_dz * (coil + 0.5));
    resultField += singleCoilField(shiftedPos, scale);
  }

  return resultField;
}

Acts::Vector2 Acts::SolenoidBField::singleCoilField(const Vector2& pos,
                                                    double scale) const {
  return {B_r(pos, scale), B_z(pos, scale)};
}

double Acts::SolenoidBField::B_r(const Vector2& pos, double scale) const {
  //              _
  //     2       /  pi / 2          2    2          - 1 / 2
  // E (k )  =   |         ( 1  -  k  sin {theta} )         dtheta
  //  1         _/  0
  using boost::math::ellint_1;
  //              _          ____________________
  //     2       /  pi / 2| /       2    2
  // E (k )  =   |        |/ 1  -  k  sin {theta} dtheta
  //  2         _/  0
  using boost::math::ellint_2;

  double r = std::abs(pos[0]);
  double z = pos[1];

  if (r == 0) {
    return 0.;
  }

  //                            _                             _
  //              mu  I        |  /     2 \                    |
  //                0     kz   |  |2 - k  |    2          2    |
  // B (r, z)  =  ----- ------ |  |-------|E (k )  -  E (k )   |
  //  r            4pi     ___ |  |      2| 2          1       |
  //                    | /  3 |_ \2 - 2k /                   _|
  //                    |/ Rr
  double k_2 = k2(r, z);
  double k = std::sqrt(k_2);
  double constant =
      scale * k * z /
      (4 * std::numbers::pi * std::sqrt(m_cfg.radius * r * r * r));

  double B = (2. - k_2) / (2. - 2. * k_2) * ellint_2(k_2) - ellint_1(k_2);

  // pos[0] is still signed!
  return r / pos[0] * constant * B;
}

double Acts::SolenoidBField::B_z(const Vector2& pos, double scale) const {
  //              _
  //     2       /  pi / 2          2    2          - 1 / 2
  // E (k )  =   |         ( 1  -  k  sin {theta} )         dtheta
  //  1         _/  0
  using boost::math::ellint_1;
  //              _          ____________________
  //     2       /  pi / 2| /       2    2
  // E (k )  =   |        |/ 1  -  k  sin {theta} dtheta
  //  2         _/  0
  using boost::math::ellint_2;

  double r = std::abs(pos[0]);
  double z = pos[1];

  //                         _                                       _
  //             mu  I      |  /         2      \                     |
  //               0     k  |  | (R + r)k  - 2r |     2          2    |
  // B (r,z)  =  ----- ---- |  | -------------- | E (k )  +  E (k )   |
  //  z           4pi    __ |  |           2    |  2          1       |
  //                   |/Rr |_ \   2r(1 - k )   /                    _|

  if (r == 0) {
    double res = scale / 2. * m_R2 / (std::sqrt(m_R2 + z * z) * (m_R2 + z * z));
    return res;
  }

  double k_2 = k2(r, z);
  double k = std::sqrt(k_2);
  double constant =
      scale * k / (4 * std::numbers::pi * std::sqrt(m_cfg.radius * r));
  double B = ((m_cfg.radius + r) * k_2 - 2. * r) / (2. * r * (1. - k_2)) *
                 ellint_2(k_2) +
             ellint_1(k_2);

  return constant * B;
}

double Acts::SolenoidBField::k2(double r, double z) const {
  //  2           4Rr
  // k   =  ---------------
  //               2      2
  //        (R + r)   +  z
  return 4 * m_cfg.radius * r /
         ((m_cfg.radius + r) * (m_cfg.radius + r) + z * z);
}
