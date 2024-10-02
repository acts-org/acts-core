// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

#include <limits>
#include <type_traits>

using namespace Acts::UnitLiterals;

static auto eps = std::numeric_limits<double>::epsilon();

BOOST_TEST_DONT_PRINT_LOG_VALUE(Acts::Neutral)
BOOST_TEST_DONT_PRINT_LOG_VALUE(Acts::SinglyCharged)
BOOST_TEST_DONT_PRINT_LOG_VALUE(Acts::NonNeutralCharge)
BOOST_TEST_DONT_PRINT_LOG_VALUE(Acts::AnyCharge)

BOOST_AUTO_TEST_SUITE(EventDataCharge)

BOOST_AUTO_TEST_CASE(Constructibility) {
  BOOST_CHECK(std::is_trivially_default_constructible_v<Acts::Neutral>);
  BOOST_CHECK(std::is_trivially_default_constructible_v<Acts::SinglyCharged>);
  BOOST_CHECK(std::is_nothrow_default_constructible_v<Acts::Neutral>);
  BOOST_CHECK(std::is_nothrow_default_constructible_v<Acts::SinglyCharged>);
  BOOST_CHECK(std::is_trivially_constructible_v<Acts::Neutral>);
  BOOST_CHECK(std::is_trivially_constructible_v<Acts::SinglyCharged>);
  // BOOST_CHECK(std::is_trivially_constructible_v<Acts::NonNeutralCharge>);
  // BOOST_CHECK(std::is_trivially_constructible_v<Acts::AnyCharge>);
  BOOST_CHECK(std::is_nothrow_constructible_v<Acts::Neutral>);
  BOOST_CHECK(std::is_nothrow_constructible_v<Acts::SinglyCharged>);
  // BOOST_CHECK(std::is_nothrow_constructible_v<Acts::NonNeutralCharge>);
  // BOOST_CHECK(std::is_nothrow_constructible_v<Acts::AnyCharge>);
}

BOOST_AUTO_TEST_CASE(Neutral) {
  Acts::Neutral q;

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 0_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 0_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), 0_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), 0_e);
  CHECK_CLOSE_REL(q.extractMomentum(1 / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(1 / 128_MeV), 128_MeV, eps);

  // negative inputs should not occur for neutral particles
  // the result is not defined, but we check it anyway
  // update: this is asserted now
  // CHECK_CLOSE_REL(q.extractMomentum(-1 / 128_MeV), -128_MeV, eps);

  BOOST_CHECK_EQUAL(q, Acts::Neutral());
  BOOST_CHECK_EQUAL(Acts::Neutral(), q);
  BOOST_CHECK_EQUAL(q, Acts::Neutral(0_e));
  BOOST_CHECK_EQUAL(Acts::Neutral(0_e), q);
}

BOOST_AUTO_TEST_CASE(SinglyCharged) {
  Acts::SinglyCharged q;

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), -1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), -1_e);
  CHECK_CLOSE_REL(q.extractMomentum(1_e / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(1_e / 128_MeV), 128_MeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(-1_e / 128_MeV), 128_MeV, eps);

  BOOST_CHECK_EQUAL(q, Acts::SinglyCharged());
  BOOST_CHECK_EQUAL(Acts::SinglyCharged(), q);
  BOOST_CHECK_EQUAL(q, Acts::SinglyCharged(1_e));
  BOOST_CHECK_EQUAL(Acts::SinglyCharged(1_e), q);
}

BOOST_AUTO_TEST_CASE(NonNeutralChargeSingle) {
  Acts::NonNeutralCharge q(1_e);

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), -1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), -1_e);
  CHECK_CLOSE_REL(q.extractMomentum(1_e / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(1_e / 128_MeV), 128_MeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(-1_e / 128_MeV), 128_MeV, eps);

  BOOST_CHECK_EQUAL(q, Acts::NonNeutralCharge(1_e));
  BOOST_CHECK_EQUAL(Acts::NonNeutralCharge(1_e), q);
}

BOOST_AUTO_TEST_CASE(NonNeutralChargeMultiple) {
  Acts::NonNeutralCharge q(3_e);

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 3_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 3_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), -3_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), -3_e);
  CHECK_CLOSE_REL(q.extractMomentum(3_e / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(3_e / 128_MeV), 128_MeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(-3_e / 128_MeV), 128_MeV, eps);

  BOOST_CHECK(!(q == Acts::NonNeutralCharge(1_e)));
  BOOST_CHECK(!(Acts::NonNeutralCharge(1_e) == q));
  BOOST_CHECK(!(q == Acts::NonNeutralCharge(2_e)));
  BOOST_CHECK(!(Acts::NonNeutralCharge(2_e) == q));
  BOOST_CHECK(q == Acts::NonNeutralCharge(3_e));
  BOOST_CHECK(Acts::NonNeutralCharge(3_e) == q);
}

BOOST_AUTO_TEST_CASE(AnyChargeNeutral) {
  Acts::AnyCharge q(0_e);

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 0_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 0_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), 0_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), 0_e);
  CHECK_CLOSE_REL(q.extractMomentum(1 / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(1 / 128_MeV), 128_MeV, eps);

  // negative inputs should not occur for neutral particles
  // the result is not defined, but we check it anyway
  CHECK_CLOSE_REL(q.extractMomentum(-1 / 128_MeV), -128_MeV, eps);

  BOOST_CHECK(q == Acts::AnyCharge(0_e));
  BOOST_CHECK(Acts::AnyCharge(0_e) == q);
  BOOST_CHECK(!(q == Acts::AnyCharge(1_e)));
  BOOST_CHECK(!(Acts::AnyCharge(1_e) == q));
  BOOST_CHECK(!(q == Acts::AnyCharge(2_e)));
  BOOST_CHECK(!(Acts::AnyCharge(2_e) == q));
}

BOOST_AUTO_TEST_CASE(AnyChargeSingle) {
  Acts::AnyCharge q(1_e);

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), -1_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), -1_e);
  CHECK_CLOSE_REL(q.extractMomentum(1_e / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(1_e / 128_MeV), 128_MeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(-1_e / 128_MeV), 128_MeV, eps);

  BOOST_CHECK(!(q == Acts::AnyCharge(0_e)));
  BOOST_CHECK(!(Acts::AnyCharge(0_e) == q));
  BOOST_CHECK(q == Acts::AnyCharge(1_e));
  BOOST_CHECK(Acts::AnyCharge(1_e) == q);
  BOOST_CHECK(!(q == Acts::AnyCharge(2_e)));
  BOOST_CHECK(!(Acts::AnyCharge(2_e) == q));
}

BOOST_AUTO_TEST_CASE(AnyChargeMultiple) {
  Acts::AnyCharge q(3_e);

  BOOST_CHECK_EQUAL(q.extractCharge(1.23), 3_e);
  BOOST_CHECK_EQUAL(q.extractCharge(2.54), 3_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-1.98), -3_e);
  BOOST_CHECK_EQUAL(q.extractCharge(-2.23), -3_e);
  CHECK_CLOSE_REL(q.extractMomentum(3_e / 64_GeV), 64_GeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(3_e / 128_MeV), 128_MeV, eps);
  CHECK_CLOSE_REL(q.extractMomentum(-3_e / 128_MeV), 128_MeV, eps);

  BOOST_CHECK(!(q == Acts::AnyCharge(0_e)));
  BOOST_CHECK(!(Acts::AnyCharge(0_e) == q));
  BOOST_CHECK(!(q == Acts::AnyCharge(1_e)));
  BOOST_CHECK(!(Acts::AnyCharge(1_e) == q));
  BOOST_CHECK(!(q == Acts::AnyCharge(2_e)));
  BOOST_CHECK(!(Acts::AnyCharge(2_e) == q));
  BOOST_CHECK(q == Acts::AnyCharge(3_e));
  BOOST_CHECK(Acts::AnyCharge(3_e) == q);
}

BOOST_AUTO_TEST_SUITE_END()
