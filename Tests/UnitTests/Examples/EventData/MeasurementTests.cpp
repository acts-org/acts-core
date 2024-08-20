// This file is part of the Acts project.
//
// Copyright (C) 2016-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/detail/GenerateParameters.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <random>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace Acts;
using namespace Acts::detail::Test;
using namespace ActsExamples;
using SourceLink = Acts::detail::Test::TestSourceLink;
namespace bd = boost::unit_test::data;

namespace {
constexpr BoundIndices boundIndices[] = {
    eBoundLoc0, eBoundLoc1, eBoundTime, eBoundPhi, eBoundTheta, eBoundQOverP,
};
const TestSourceLink sourceOrig;
const Acts::SourceLink source{sourceOrig};
// fix seed for reproducible tests
std::default_random_engine rng(123);
}  // namespace

// the underlying subspace implementation is already tested exhaustively in a
// separate unit test. here we only test concrete extreme cases and
// measurement-specific functionality.

BOOST_AUTO_TEST_SUITE(EventDataMeasurement)

BOOST_DATA_TEST_CASE(VariableBoundOne, bd::make(boundIndices), index) {
  auto [params, cov] = generateParametersCovariance<ActsScalar, 1u>(rng);
  auto meas = makeVariableSizeMeasurement(source, params, cov, index);

  BOOST_CHECK_EQUAL(meas.size(), 1);
  for (auto i : boundIndices) {
    if (i == index) {
      BOOST_CHECK(meas.contains(i));
    } else {
      BOOST_CHECK(!meas.contains(i));
    }
  }
  BOOST_CHECK_EQUAL(meas.parameters(), params);
  BOOST_CHECK_EQUAL(meas.covariance(), cov);
  BOOST_CHECK_EQUAL(meas.sourceLink().template get<TestSourceLink>(),
                    sourceOrig);
}

BOOST_AUTO_TEST_CASE(VariableBoundAll) {
  auto [params, cov] = generateBoundParametersCovariance(rng);
  auto meas = makeVariableSizeMeasurement(source, params, cov, eBoundLoc0,
                                          eBoundLoc1, eBoundPhi, eBoundTheta,
                                          eBoundQOverP, eBoundTime);

  BOOST_CHECK_EQUAL(meas.size(), eBoundSize);
  for (auto i : boundIndices) {
    BOOST_CHECK(meas.contains(i));
  }
  BOOST_CHECK_EQUAL(meas.parameters(), params);
  BOOST_CHECK_EQUAL(meas.covariance(), cov);
  BOOST_CHECK_EQUAL(meas.sourceLink().get<TestSourceLink>(), sourceOrig);
}

BOOST_AUTO_TEST_CASE(VariableBoundReassign) {
  // generate w/ a single parameter
  auto [par1, cov1] = generateParametersCovariance<ActsScalar, 1u>(rng);
  auto meas = makeVariableSizeMeasurement(source, par1, cov1, eBoundTheta);
  BOOST_CHECK_EQUAL(meas.size(), 1);
  BOOST_CHECK(!meas.contains(eBoundLoc0));
  BOOST_CHECK(!meas.contains(eBoundLoc1));
  BOOST_CHECK(!meas.contains(eBoundTime));
  BOOST_CHECK(!meas.contains(eBoundPhi));
  BOOST_CHECK(meas.contains(eBoundTheta));
  BOOST_CHECK(!meas.contains(eBoundQOverP));

  // reassign w/ all parameters
  auto [parN, covN] = generateBoundParametersCovariance(rng);
  meas = makeVariableSizeMeasurement(source, parN, covN, eBoundLoc0, eBoundLoc1,
                                     eBoundPhi, eBoundTheta, eBoundQOverP,
                                     eBoundTime);
  BOOST_CHECK_EQUAL(meas.size(), eBoundSize);
  BOOST_CHECK(meas.contains(eBoundLoc0));
  BOOST_CHECK(meas.contains(eBoundLoc1));
  BOOST_CHECK(meas.contains(eBoundTime));
  BOOST_CHECK(meas.contains(eBoundPhi));
  BOOST_CHECK(meas.contains(eBoundTheta));
  BOOST_CHECK(meas.contains(eBoundQOverP));
}

BOOST_AUTO_TEST_SUITE_END()
