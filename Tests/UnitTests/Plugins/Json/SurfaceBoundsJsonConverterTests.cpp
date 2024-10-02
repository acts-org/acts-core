// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Plugins/Json/SurfaceBoundsJsonConverter.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using namespace Acts;

BOOST_AUTO_TEST_SUITE(SurfaceBoundsJsonConversion)

BOOST_AUTO_TEST_CASE(SurfaceBoundsRoundTripTests) {
  std::ofstream out;

  // As all SurfaceBounds have the same streaming API only a one is
  // tested here, all others are tests are identical

  auto rectangeRef = std::make_shared<const RectangleBounds>(4., 6.);
  nlohmann::json rectangleOut =
      SurfaceBoundsJsonConverter::toJson(*rectangeRef);
  out.open("RectangleBounds.json");
  out << rectangleOut.dump(2);
  out.close();

  auto in = std::ifstream("RectangleBounds.json",
                          std::ifstream::in | std::ifstream::binary);
  BOOST_CHECK(in.good());
  nlohmann::json rectangleIn;
  in >> rectangleIn;
  in.close();

  auto rectangleTest =
      SurfaceBoundsJsonConverter::fromJson<RectangleBounds>(rectangleIn);

  BOOST_CHECK(rectangeRef->values() == rectangleTest->values());
}

BOOST_AUTO_TEST_CASE(SurfaceBoundsDetrayConversion) {
  auto rectangeRef = std::make_shared<const RectangleBounds>(4., 6.);
  nlohmann::json rectangleOutDetray =
      SurfaceBoundsJsonConverter::toJsonDetray(*rectangeRef);

  std::vector<ActsScalar> boundariesRef = {4, 6};
  BOOST_CHECK_EQUAL(rectangleOutDetray["shape"].get<unsigned int>(), 5u);
  BOOST_CHECK(rectangleOutDetray["boundaries"].get<std::vector<ActsScalar>>() ==
              boundariesRef);
}

BOOST_AUTO_TEST_SUITE_END()
