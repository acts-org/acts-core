// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/TrackParameterHelpers.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"

BOOST_AUTO_TEST_SUITE(TrackParameterHelpers)

BOOST_AUTO_TEST_CASE(isBoundVectorValid) {
  BOOST_CHECK(!Acts::isBoundVectorValid({1, 2, 3, 4, 5, 6}, true));
  BOOST_CHECK(Acts::isBoundVectorValid({1, 2, 1, 1, 5, 6}, true));
}

BOOST_AUTO_TEST_CASE(isFreeVectorValid) {
  BOOST_CHECK(!Acts::isFreeVectorValid({1, 2, 3, 4, 5, 6, 7, 8}));
  BOOST_CHECK(Acts::isFreeVectorValid({1, 2, 3, 4, 1, 0, 0, 8}));
}

BOOST_AUTO_TEST_CASE(normalizeBoundParameters) {
  CHECK_CLOSE_OR_SMALL(Acts::normalizeBoundParameters({1, 2, 3, 4, 5, 6}),
                       Acts::BoundVector(1, 2, -0.141593, 2.28319, 5, 6), 1e-3,
                       1e-3);
}

BOOST_AUTO_TEST_CASE(addBoundParameters) {
  CHECK_CLOSE_OR_SMALL(
      Acts::addBoundParameters({1, 2, 3, 4, 5, 6}, {0, 0, 0, 0, 0, 0}),
      Acts::normalizeBoundParameters({1, 2, 3, 4, 5, 6}), 1e-3, 1e-3);
  CHECK_CLOSE_OR_SMALL(
      Acts::addBoundParameters({1, 2, 3, 4, 5, 6}, {0, 0, 1, 1, 0, 0}),
      Acts::normalizeBoundParameters({1, 2, 4, 5, 5, 6}), 1e-3, 1e-3);
}

BOOST_AUTO_TEST_CASE(subtractBoundParameters) {
  CHECK_CLOSE_OR_SMALL(
      Acts::subtractBoundParameters({1, 2, 3, 4, 5, 6}, {1, 2, 3, 4, 5, 6}),
      Acts::BoundVector(0, 0, 0, 0, 0, 0), 1e-3, 1e-3);
  CHECK_CLOSE_OR_SMALL(
      Acts::addBoundParameters(
          Acts::subtractBoundParameters({1, 2, 3, 4, 5, 6}, {0, 0, 1, 1, 0, 0}),
          {0, 0, 1, 1, 0, 0}),
      Acts::normalizeBoundParameters({1, 2, 3, 4, 5, 6}), 1e-3, 1e-3);
}

BOOST_AUTO_TEST_SUITE_END()
