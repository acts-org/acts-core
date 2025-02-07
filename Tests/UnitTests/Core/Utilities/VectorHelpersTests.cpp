// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <numbers>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::theta;

BOOST_AUTO_TEST_SUITE(AngleHelpers)

BOOST_AUTO_TEST_CASE(EtaFromVector) {
  CHECK_CLOSE_ABS(0.0, eta(Acts::Vector3{1, 0, 0}), 1e-6);
}

BOOST_AUTO_TEST_CASE(ThetaFromVector) {
  CHECK_CLOSE_ABS(std::numbers::pi / 2, theta(Acts::Vector3{1, 0, 0}), 1e-6);
}

BOOST_AUTO_TEST_SUITE_END()
