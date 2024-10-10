// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/SolenoidBField.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/Result.hpp"

#include <cstddef>
#include <fstream>

using namespace Acts::UnitLiterals;

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(TestSolenoidBField) {
  // Create a test context
  MagneticFieldContext mfContext = MagneticFieldContext();

  SolenoidBField::Config cfg{};
  cfg.length = 5.8_m;
  cfg.radius = (2.56 + 2.46) * 0.5 * 0.5_m;
  cfg.nCoils = 1154;
  cfg.bMagCenter = 2_T;
  SolenoidBField bField(cfg);

  auto cache = bField.makeCache(mfContext);
  CHECK_CLOSE_ABS(bField.getField({0, 0, 0}, cache).value(),
                  Vector3(0, 0, 2.0_T), 1e-6_T);

  // std::ofstream outf("solenoid.csv");
  // outf << "x;y;z;B_x;B_y;B_z" << std::endl;

  double tol = 1e-6;
  double tol_B = 1e-6_T;
  std::size_t steps = 20;
  for (std::size_t i = 0; i < steps; i++) {
    double r = 1.5 * cfg.radius / steps * i;
    BOOST_TEST_CONTEXT("r=" << r) {
      Vector3 B1 = bField.getField({r, 0, 0}, cache).value();
      Vector3 B2 = bField.getField({-r, 0, 0}, cache).value();
      CHECK_SMALL(B1.x(), tol);
      CHECK_SMALL(B1.y(), tol);
      BOOST_CHECK_GT(std::abs(B1.z()), tol_B);  // greater than zero
      // check symmetry: at z=0 it should be exactly symmetric
      CHECK_CLOSE_ABS(B1, B2, tol_B);

      // at this point in r, go along the length
      for (std::size_t j = 0; j <= steps; j++) {
        // double z = cfg.L/steps * j - (cfg.L/2.);
        double z = (1.5 * cfg.length / 2.) / steps * j;
        BOOST_TEST_CONTEXT("z=" << z) {
          Vector3 B_zp_rp = bField.getField({r, 0, z}, cache).value();
          Vector3 B_zn_rp = bField.getField({r, 0, -z}, cache).value();
          Vector3 B_zp_rn = bField.getField({-r, 0, z}, cache).value();
          Vector3 B_zn_rn = bField.getField({-r, 0, -z}, cache).value();

          // outf << r << ";0;" << z << ";" << B_zp_rp.x() << ";" <<
          // B_zp_rp.y() << ";" << B_zp_rp.z() << std::endl;
          // if(j>0) {
          // outf << r << ";0;" << -z << ";" << B_zn_rp.x() << ";" <<
          // B_zn_rp.y() << ";" << B_zn_rp.z() << std::endl;
          //}
          // if(i>0) {
          // outf << -r << ";0;" << z << ";" << B_zp_rn.x() << ";" <<
          // B_zp_rn.y() << ";" << B_zp_rn.z() << std::endl;
          //}
          // if(i>0 && j>0) {
          // outf << -r << ";0;" << -z << ";" << B_zn_rn.x() << ";" <<
          // B_zn_rn.y() << ";" << B_zn_rn.z() << std::endl;
          //}

          // non-zero z
          BOOST_CHECK_GT(std::abs(B_zp_rp.z()), tol_B);
          BOOST_CHECK_GT(std::abs(B_zn_rp.z()), tol_B);
          BOOST_CHECK_GT(std::abs(B_zn_rn.z()), tol_B);
          BOOST_CHECK_GT(std::abs(B_zp_rn.z()), tol_B);
          if (i > 0) {
            // z components should be the same for +- r
            CHECK_CLOSE_ABS(B_zp_rp.z(), B_zp_rn.z(), tol_B);
            CHECK_CLOSE_ABS(B_zn_rp.z(), B_zn_rn.z(), tol_B);
            // x components should be exactly opposite
            CHECK_CLOSE_ABS(B_zp_rp.x(), -B_zp_rn.x(), tol_B);
            CHECK_CLOSE_ABS(B_zn_rp.x(), -B_zn_rn.x(), tol_B);
          }
          if (j > 0) {
            // z components should be the same for +- z
            CHECK_CLOSE_ABS(B_zp_rp.z(), B_zn_rp.z(), tol_B);
            CHECK_CLOSE_ABS(B_zp_rn.z(), B_zn_rn.z(), tol_B);
            // x components should be exactly opposite
            CHECK_CLOSE_ABS(B_zp_rp.x(), -B_zn_rp.x(), tol_B);
            CHECK_CLOSE_ABS(B_zp_rn.x(), -B_zn_rn.x(), tol_B);
          }
        }
      }
    }
  }
  // outf.close();
}

}  // namespace Acts::Test
