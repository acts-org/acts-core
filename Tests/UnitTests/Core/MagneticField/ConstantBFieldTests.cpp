// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Utilities/Result.hpp"

#include <utility>

namespace bdata = boost::unit_test::data;
using namespace Acts::UnitLiterals;

namespace Acts::Test {

// Create a test context
MagneticFieldContext mfContext = MagneticFieldContext();

/// @brief unit test for construction of constant magnetic field
///
/// Tests the correct behavior and consistency of
/// -# ConstantBField::ConstantBField(double Bx,double By,double Bz)
/// -# ConstantBField::ConstantBField(Vector3 B)
/// -# ConstantBField::getField(const double* xyz, double* B) const
/// -# ConstantBField::getField(const Vector3& pos) const
BOOST_DATA_TEST_CASE(
    ConstantBField_components,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                   bdata::distribution =
                       std::uniform_real_distribution<double>(-2_T, 2_T))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-1_T, 4_T))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0_T, 10_T))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10_m,
                                                                  10_m))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 5,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10_m,
                                                                  10_m))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 6,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10_m,
                                                                  10_m))) ^
        bdata::xrange(10),
    x, y, z, bx, by, bz, index) {
  (void)index;
  const Vector3 Btrue(bx, by, bz);
  const Vector3 pos(x, y, z);
  const ConstantBField BField(Btrue);

  auto bCache = BField.makeCache(mfContext);

  BOOST_CHECK_EQUAL(Btrue, BField.getField());

  BOOST_CHECK_EQUAL(Btrue, BField.getField(pos, bCache).value());
  BOOST_CHECK_EQUAL(Btrue, BField.getField(Vector3(0, 0, 0), bCache).value());
  BOOST_CHECK_EQUAL(Btrue, BField.getField(-2 * pos, bCache).value());
}

/// @brief unit test for update of constant magnetic field
///
/// Tests the correct behavior and consistency of
/// -# ConstantBField::setField(double Bx, double By, double Bz)
/// -# ConstantBField::setField(const Vector3& B)
/// -# ConstantBField::getField(const double* xyz, double* B) const
/// -# ConstantBField::getField(const Vector3& pos) const
BOOST_DATA_TEST_CASE(
    ConstantBField_update,
    bdata::random((bdata::engine = std::mt19937(), bdata::seed = 1,
                   bdata::distribution =
                       std::uniform_real_distribution<double>(-2_T, 2_T))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 2,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-1_T, 4_T))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 3,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(0_T, 10_T))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 4,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10_m,
                                                                  10_m))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 5,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10_m,
                                                                  10_m))) ^
        bdata::random((bdata::engine = std::mt19937(), bdata::seed = 6,
                       bdata::distribution =
                           std::uniform_real_distribution<double>(-10_m,
                                                                  10_m))) ^
        bdata::xrange(10),
    x, y, z, bx, by, bz, index) {
  (void)index;

  ConstantBField BField{Vector3{0, 0, 0}};
  const Vector3 Btrue(bx, by, bz);
  const Vector3 pos(x, y, z);
  BField.setField(Vector3{bx, by, bz});

  auto bCache = BField.makeCache(mfContext);

  BOOST_CHECK_EQUAL(Btrue, BField.getField());

  BOOST_CHECK_EQUAL(Btrue, BField.getField(pos, bCache).value());
  BOOST_CHECK_EQUAL(Btrue, BField.getField(Vector3(0, 0, 0), bCache).value());
  BOOST_CHECK_EQUAL(Btrue, BField.getField(-2 * pos, bCache).value());
}

}  // namespace Acts::Test
