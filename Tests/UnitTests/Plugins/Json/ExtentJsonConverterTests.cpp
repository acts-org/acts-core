// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Plugins/Json/ExtentJsonConverter.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <nlohmann/json.hpp>

using namespace Acts;

BOOST_AUTO_TEST_SUITE(ExtentJsonConverter)

BOOST_AUTO_TEST_CASE(ExtentRoundtripTests) {
  Extent e;
  e.set(BinningValue::binR, 0, 200);
  e.set(BinningValue::binZ, -50, 50);

  nlohmann::json j;
  j["extent"] = e;

  std::cout << j.dump(2) << std::endl;

  Extent eIn = j["extent"];

  CHECK_CLOSE_ABS(eIn.min(BinningValue::binR), e.min(BinningValue::binR),
                  10e-5);
  CHECK_CLOSE_ABS(eIn.max(BinningValue::binR), e.max(BinningValue::binR),
                  10e-5);
  CHECK_CLOSE_ABS(eIn.min(BinningValue::binZ), e.min(BinningValue::binZ),
                  10e-5);
  CHECK_CLOSE_ABS(eIn.max(BinningValue::binZ), e.max(BinningValue::binZ),
                  10e-5);
}

BOOST_AUTO_TEST_SUITE_END()
