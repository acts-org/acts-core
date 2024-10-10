// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/EventData/detail/TestSourceLink.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"

#include <any>
#include <sstream>

using namespace Acts::UnitLiterals;

BOOST_AUTO_TEST_SUITE(EventDataSourceLink)

BOOST_AUTO_TEST_CASE(TestSourceLinkCoverage) {
  using Acts::detail::Test::TestSourceLink;

  TestSourceLink ts;
  Acts::Vector2 stddev(0.01, 0.1);
  Acts::SquareMatrix2 cov = stddev.cwiseProduct(stddev).asDiagonal();
  TestSourceLink l1(Acts::eBoundLoc0, 0.1, cov(0, 0),
                    Acts::GeometryIdentifier(0x999), 0);
  TestSourceLink l2(l1);

  BOOST_CHECK(l1 == l2);     // testing the ==
  BOOST_CHECK(!(l1 != l2));  // testing the !=
  std::ostringstream str;
  str << l1;
}

struct MySourceLink {
  Acts::GeometryIdentifier m_geometryId;

  Acts::GeometryIdentifier geometryId() const { return m_geometryId; }
};

BOOST_AUTO_TEST_CASE(Construct) {
  MySourceLink msl;
  msl.m_geometryId.setSensitive(42);
  {
    Acts::SourceLink sl{msl};
    BOOST_CHECK_EQUAL(sl.get<MySourceLink>().geometryId(), msl.geometryId());
    BOOST_CHECK_THROW(sl.get<int>(), std::bad_any_cast);
  }
}

BOOST_AUTO_TEST_CASE(Reassign) {
  int value = 5;
  Acts::SourceLink sl{value};

  BOOST_CHECK_EQUAL(sl.get<int>(), value);
  BOOST_CHECK_THROW(sl.get<double>(), std::bad_any_cast);

  double otherValue = 42.42;

  // this changes the stored type
  sl = Acts::SourceLink{otherValue};
  BOOST_CHECK_EQUAL(sl.get<double>(), otherValue);
  BOOST_CHECK_THROW(sl.get<int>(), std::bad_any_cast);
}

BOOST_AUTO_TEST_SUITE_END()
