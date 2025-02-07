// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"

#include <cmath>
#include <numbers>

namespace bd = boost::unit_test::data;

using Acts::AngleHelpers::etaFromTheta;
using Acts::AngleHelpers::EtaThetaConversionTraits;
using Acts::AngleHelpers::thetaFromEta;

BOOST_AUTO_TEST_SUITE(AngleHelpers)

BOOST_AUTO_TEST_CASE(EtaThetaConversion) {
  CHECK_CLOSE_ABS(0.0, etaFromTheta(std::numbers::pi / 2), 1e-6);
  CHECK_CLOSE_ABS(1.0, etaFromTheta(thetaFromEta(1.0)), 1e-6);
  CHECK_CLOSE_ABS(1.0, thetaFromEta(etaFromTheta(1.0)), 1e-6);
}

BOOST_DATA_TEST_CASE(EtaFromThetaRobustness, bd::xrange(0, 1000, 1), exponent) {
  {
    // check right

    float thetaRight = exponent < 30 ? std::pow(10.0f, -1.0f * exponent) : 0.0f;

    float etaRight = etaFromTheta<float>(thetaRight);
    BOOST_CHECK(!std::isnan(etaRight));

    // check left

    float thetaLeft = std::numbers::pi_v<float> - thetaRight;

    float etaLeft = etaFromTheta<float>(thetaLeft);
    BOOST_CHECK(!std::isnan(etaLeft));
  }

  {
    // check right

    double thetaRight = exponent < 300 ? std::pow(10.0, -1.0 * exponent) : 0.0;

    double etaRight = etaFromTheta<double>(thetaRight);
    BOOST_CHECK(!std::isnan(etaRight));

    // check left

    double thetaLeft = std::numbers::pi - thetaRight;

    double etaLeft = etaFromTheta<double>(thetaLeft);
    BOOST_CHECK(!std::isnan(etaLeft));
  }
}

BOOST_DATA_TEST_CASE(ThetaFromEtaRobustness, bd::xrange(1.0, 1000.0, 1.0),
                     etaRight) {
  {
    // check right

    float thetaRight = thetaFromEta<float>(etaRight);
    BOOST_CHECK(!std::isnan(thetaRight));

    // check left

    float etaLeft = -etaRight;

    float thetaLeft = thetaFromEta<float>(etaLeft);
    BOOST_CHECK(!std::isnan(thetaLeft));
  }

  {
    // check right

    double thetaRight = thetaFromEta<double>(etaRight);
    BOOST_CHECK(!std::isnan(thetaRight));

    // check left

    double etaLeft = -etaRight;

    double thetaLeft = thetaFromEta<double>(etaLeft);
    BOOST_CHECK(!std::isnan(thetaLeft));
  }
}

BOOST_AUTO_TEST_SUITE_END()
