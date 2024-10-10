// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"

#include <string>
#include <vector>

namespace Acts::Test {

/// @brief Unit test checking the match probability
BOOST_AUTO_TEST_CASE(TGeoPrimitivesHelper_match) {
  BOOST_CHECK(TGeoPrimitivesHelper::match("Pixel*Barrel", "PixelLBarrel"));
  BOOST_CHECK(!TGeoPrimitivesHelper::match("Pixel*Barrel", "Strips"));

  std::vector<std::string> candidates = {"Pixel", "Barrel*", "*P*pe"};
  BOOST_CHECK(TGeoPrimitivesHelper::match(candidates, "BarrelStrips"));
  BOOST_CHECK(TGeoPrimitivesHelper::match(candidates, "BeamPipe"));
}

}  // namespace Acts::Test
