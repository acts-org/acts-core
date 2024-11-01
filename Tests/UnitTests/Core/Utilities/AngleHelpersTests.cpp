// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/AngleHelpers.hpp"

#include <cmath>
#include <numbers>

namespace bd = boost::unit_test::data;

using Acts::AngleHelpers::etaFromTheta;
using Acts::AngleHelpers::etaFromThetaClamped;
using Acts::AngleHelpers::thetaFromEta;
using Acts::AngleHelpers::thetaFromEtaClamped;

using Acts::AngleHelpers::ClampedEtaThetaConversionTraits;

BOOST_AUTO_TEST_SUITE(AngleHelpers)

BOOST_AUTO_TEST_CASE(EtaThetaConversion) {
  CHECK_CLOSE_ABS(0.0, etaFromTheta(std::numbers::pi / 2), 1e-6);
  CHECK_CLOSE_ABS(1.0, etaFromTheta(thetaFromEta(1.0)), 1e-6);
  CHECK_CLOSE_ABS(1.0, thetaFromEta(etaFromTheta(1.0)), 1e-6);

  CHECK_CLOSE_ABS(0.0, etaFromThetaClamped(std::numbers::pi / 2), 1e-6);
  CHECK_CLOSE_ABS(1.0, etaFromThetaClamped(thetaFromEta(1.0)), 1e-6);
  CHECK_CLOSE_ABS(1.0, thetaFromEtaClamped(etaFromTheta(1.0)), 1e-6);

  // test clamped limits

  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<double>::maxTheta,
                    thetaFromEtaClamped<double>(
                        ClampedEtaThetaConversionTraits<double>::minEta));
  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<double>::minTheta,
                    thetaFromEtaClamped<double>(
                        ClampedEtaThetaConversionTraits<double>::maxEta));

  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<double>::minEta,
                    etaFromThetaClamped<double>(
                        ClampedEtaThetaConversionTraits<double>::maxTheta));
  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<double>::maxEta,
                    etaFromThetaClamped<double>(
                        ClampedEtaThetaConversionTraits<double>::minTheta));

  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<float>::maxTheta,
                    thetaFromEtaClamped<float>(
                        ClampedEtaThetaConversionTraits<float>::minEta));
  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<float>::minTheta,
                    thetaFromEtaClamped<float>(
                        ClampedEtaThetaConversionTraits<float>::maxEta));

  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<float>::minEta,
                    etaFromThetaClamped<float>(
                        ClampedEtaThetaConversionTraits<float>::maxTheta));
  BOOST_CHECK_EQUAL(ClampedEtaThetaConversionTraits<float>::maxEta,
                    etaFromThetaClamped<float>(
                        ClampedEtaThetaConversionTraits<float>::minTheta));
}

BOOST_DATA_TEST_CASE(EtaFromThetaRobustness, bd::xrange(0, 1000, 1), exponent) {
  {
    // check right

    float thetaRight = exponent < 30 ? std::pow(10.0f, -1.0f * exponent) : 0.0f;

    float etaRight = etaFromTheta<float>(thetaRight);
    BOOST_CHECK(!std::isnan(etaRight));

    float etaRightClamped = etaFromThetaClamped<float>(thetaRight);
    BOOST_CHECK(!std::isnan(etaRightClamped));

    // check left

    float thetaLeft = std::numbers::pi_v<float> - thetaRight;

    float etaLeft = etaFromTheta<float>(thetaLeft);
    BOOST_CHECK(!std::isnan(etaLeft));

    float etaLeftClamped = etaFromThetaClamped<float>(thetaLeft);
    BOOST_CHECK(!std::isnan(etaLeftClamped));
  }

  {
    // check right

    double thetaRight = exponent < 300 ? std::pow(10.0, -1.0 * exponent) : 0.0;

    double etaRight = etaFromTheta<double>(thetaRight);
    BOOST_CHECK(!std::isnan(etaRight));

    double etaRightClamped = etaFromThetaClamped<double>(thetaRight);
    BOOST_CHECK(!std::isnan(etaRightClamped));

    // check left

    double thetaLeft = std::numbers::pi - thetaRight;

    double etaLeft = etaFromTheta<double>(thetaLeft);
    BOOST_CHECK(!std::isnan(etaLeft));

    double etaLeftClamped = etaFromThetaClamped<double>(thetaLeft);
    BOOST_CHECK(!std::isnan(etaLeftClamped));
  }
}

BOOST_DATA_TEST_CASE(ThetaFromEtaRobustness, bd::xrange(1.0, 1000.0, 1.0),
                     etaRight) {
  {
    // check right

    float thetaRight = thetaFromEta<float>(etaRight);
    BOOST_CHECK(!std::isnan(thetaRight));

    float thetaRightClamped = thetaFromEtaClamped<float>(etaRight);
    BOOST_CHECK(!std::isnan(thetaRightClamped));

    // check left

    float etaLeft = -etaRight;

    float thetaLeft = thetaFromEta<float>(etaLeft);
    BOOST_CHECK(!std::isnan(thetaLeft));

    float thetaLeftClamped = thetaFromEtaClamped<float>(etaLeft);
    BOOST_CHECK(!std::isnan(thetaLeftClamped));
  }

  {
    // check right

    double thetaRight = thetaFromEta<double>(etaRight);
    BOOST_CHECK(!std::isnan(thetaRight));

    double thetaRightClamped = thetaFromEtaClamped<double>(etaRight);
    BOOST_CHECK(!std::isnan(thetaRightClamped));

    // check left

    double etaLeft = -etaRight;

    double thetaLeft = thetaFromEta<double>(etaLeft);
    BOOST_CHECK(!std::isnan(thetaLeft));

    double thetaLeftClamped = thetaFromEtaClamped<double>(etaLeft);
    BOOST_CHECK(!std::isnan(thetaLeftClamped));
  }
}

BOOST_AUTO_TEST_SUITE_END()
