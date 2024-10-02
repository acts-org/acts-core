// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MeasurementHelpers.hpp"

BOOST_AUTO_TEST_SUITE(EventDataMeasurementHelpers)

BOOST_AUTO_TEST_CASE(visit_measurement_test) {
  // Overallocated full size parameter vector and covariance
  Acts::BoundVector parFull = Acts::BoundVector::Random();
  Acts::BoundMatrix covFull = Acts::BoundMatrix::Random();
  // constant variants
  const auto& parFullConst = parFull;
  const auto& covFullConst = covFull;

  for (Acts::BoundVector::Index dim = 1; dim <= parFull.size(); ++dim) {
    Acts::visit_measurement(parFull, covFull, dim, [&](auto param, auto cov) {
      BOOST_CHECK_EQUAL(param, parFull.head(dim));
      BOOST_CHECK_EQUAL(cov, covFull.topLeftCorner(dim, dim));
    });
    Acts::visit_measurement(
        parFull, covFull, dim, [&](const auto& param, const auto& cov) {
          BOOST_CHECK_EQUAL(param, parFull.head(dim));
          BOOST_CHECK_EQUAL(cov, covFull.topLeftCorner(dim, dim));
        });
    Acts::visit_measurement(parFullConst, covFullConst, dim,
                            [&](const auto& param, const auto& cov) {
                              BOOST_CHECK_EQUAL(param, parFullConst.head(dim));
                              BOOST_CHECK_EQUAL(
                                  cov, covFullConst.topLeftCorner(dim, dim));
                            });
  }
}

BOOST_AUTO_TEST_SUITE_END()
