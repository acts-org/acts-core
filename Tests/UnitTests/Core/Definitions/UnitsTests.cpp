// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <cmath>
#include <limits>

using namespace Acts::UnitLiterals;

static constexpr auto eps = std::numeric_limits<double>::epsilon();

BOOST_AUTO_TEST_SUITE(DefinitionsUnits)

BOOST_AUTO_TEST_CASE(Length) {
  CHECK_CLOSE_REL(1_m, 1e-3_km, eps);
  CHECK_CLOSE_REL(1_m, 1e2_cm, eps);
  CHECK_CLOSE_REL(1_m, 1e3_mm, eps);
  CHECK_CLOSE_REL(1_m, 1e6_um, eps);
  CHECK_CLOSE_REL(1_m, 1e9_nm, eps);
  CHECK_CLOSE_REL(1_m, 1e12_pm, eps);
  CHECK_CLOSE_REL(1_m, 1e15_fm, eps);
}

BOOST_AUTO_TEST_CASE(Area) {
  CHECK_CLOSE_REL(1_mm * 1_mm, 1_mm2, eps);
  CHECK_CLOSE_REL(1_mm * 1_mm, 0.01_cm2, eps);
  CHECK_CLOSE_REL(1_mm * 1_mm, 0.000001_m2, eps);
  CHECK_CLOSE_REL(1_cm * 1_cm, 100_mm2, eps);
  CHECK_CLOSE_REL(1_cm * 1_cm, 1_cm2, eps);
  CHECK_CLOSE_REL(1_cm * 1_cm, 0.0001_m2, eps);
  CHECK_CLOSE_REL(1_m * 1_m, 1000000_mm2, eps);
  CHECK_CLOSE_REL(1_m * 1_m, 10000_cm2, eps);
  CHECK_CLOSE_REL(1_m * 1_m, 1_m2, eps);
}

BOOST_AUTO_TEST_CASE(Volume) {
  CHECK_CLOSE_REL(1_mm * 1_mm * 1_mm, 1_mm3, eps);
  CHECK_CLOSE_REL(1_mm2 * 1_mm, 1_mm3, eps);
  CHECK_CLOSE_REL(1_cm * 1_cm * 1_cm, 1_cm3, eps);
  CHECK_CLOSE_REL(1_cm2 * 1_cm, 1_cm3, eps);
  CHECK_CLOSE_REL(1_m * 1_m * 1_m, 1_m3, eps);
  CHECK_CLOSE_REL(1_m2 * 1_m, 1_m3, eps);
}

BOOST_AUTO_TEST_CASE(Time) {
  CHECK_CLOSE_REL(1_h, 60_min, eps);
  CHECK_CLOSE_REL(1_h, 3600_s, eps);
  CHECK_CLOSE_REL(1_min, 60_s, eps);
  CHECK_CLOSE_REL(1_s, 1e3_ms, eps);
  CHECK_CLOSE_REL(1_s, 1e6_us, eps);
  CHECK_CLOSE_REL(1_s, 1e9_ns, eps);
  CHECK_CLOSE_REL(1_s, 1e12_ps, eps);
  CHECK_CLOSE_REL(1_s, 1e15_fs, eps);
}

BOOST_AUTO_TEST_CASE(Angle) {
  CHECK_CLOSE_REL(45_degree, M_PI / 4 * 1_rad, eps);
  CHECK_CLOSE_REL(90_degree, M_PI / 2 * 1_rad, eps);
  CHECK_CLOSE_REL(180_degree, M_PI * 1_rad, eps);
  CHECK_CLOSE_REL(360_degree, 2 * M_PI * 1_rad, eps);
  CHECK_CLOSE_REL(1_mm / 1_m, 1_mrad, eps);
  CHECK_CLOSE_REL(1_um / 1_mm, 1_mrad, eps);
}

BOOST_AUTO_TEST_CASE(Energy) {
  CHECK_CLOSE_REL(1_MeV, 1e6_eV, eps);
  CHECK_CLOSE_REL(1_MeV, 1e3_keV, eps);
  CHECK_CLOSE_REL(1_MeV, 1e-3_GeV, eps);
  CHECK_CLOSE_REL(1_MeV, 1e-6_TeV, eps);
}

BOOST_AUTO_TEST_CASE(Mass) {
  // always assume c == 1
  CHECK_CLOSE_REL(1_kg, 1000_g, eps);
  CHECK_CLOSE_REL(0.001_kg, 1_g, eps);
  CHECK_CLOSE_REL(1_u, 931.49410242_MeV, eps);
  CHECK_CLOSE_REL(1_u, 1.66053906660e-24_g, 1e-7);
}

BOOST_AUTO_TEST_CASE(MassEnergy) {
  // always assume c == 1
  CHECK_CLOSE_REL(1.782662e-36_kg, 1_eV, eps);
  CHECK_CLOSE_REL(1.782662e-33_kg, 1_keV, eps);
  CHECK_CLOSE_REL(1.782662e-30_kg, 1_MeV, eps);
  CHECK_CLOSE_REL(1.782662e-27_kg, 1_GeV, eps);
  CHECK_CLOSE_REL(1.782662e-33_g, 1_eV, eps);
  CHECK_CLOSE_REL(1.782662e-30_g, 1_keV, eps);
  CHECK_CLOSE_REL(1.782662e-27_g, 1_MeV, eps);
  CHECK_CLOSE_REL(1.782662e-24_g, 1_GeV, eps);
}

BOOST_AUTO_TEST_CASE(DecayWidthTime) {
  using Acts::PhysicalConstants::hbar;
  // lifetime is hbar / decay-width
  // pion
  CHECK_CLOSE_REL(hbar / 2.5284e-17_GeV, 26.032746062598505_ns, 1e-7);
  // muon
  CHECK_CLOSE_REL(hbar / 2.9959847e-19_GeV, 2.1969803498887713_us, 1e-7);
  // top
  CHECK_CLOSE_REL(hbar / 1.42_GeV, 4.635295432723526e-10_fs, 1e-7);
}

BOOST_AUTO_TEST_CASE(MagneticField) {
  CHECK_CLOSE_REL(10_kGauss, 1_T, eps);
  CHECK_CLOSE_REL(1_kGauss, 1000_Gauss, eps);
}

BOOST_AUTO_TEST_CASE(MomentumRadius) {
  // note: no conversion factors necessary
  CHECK_CLOSE_REL(1_GeV / (1_e * 1_T), 3.3336_m, 1e-3);
  CHECK_CLOSE_REL(1_GeV / (1_e * 2_T), 166.8_cm, 1e-3);
  CHECK_CLOSE_REL(1_GeV / (2_e * 1_T), 166.8_cm, 1e-3);
  CHECK_CLOSE_REL(1_GeV / (1_e * 4_T), 83.39_cm, 1e-3);
  CHECK_CLOSE_REL(1_GeV / (2_e * 2_T), 83.39_cm, 1e-3);
}

BOOST_AUTO_TEST_CASE(PhysicalConstants) {
  using Acts::PhysicalConstants::hbar;
  // see https://en.wikipedia.org/wiki/Planck_constant
  CHECK_CLOSE_REL(hbar, 6.62607015e-34 * 1_J * 1_s / (2 * M_PI), 1e-6);
  CHECK_CLOSE_REL(hbar, 4.135667696e-15 * 1_eV * 1_s / (2 * M_PI), 1e-7);

  using Acts::PhysicalConstants::c;
  // we really want c to be 1
  BOOST_CHECK_EQUAL(c, 1.0);
}

BOOST_AUTO_TEST_SUITE_END()
