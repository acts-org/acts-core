// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include <Acts/Utilities/Zip.hpp>

#include <array>
#include <string>
#include <vector>

const std::vector<int> vec = {1, 2, 3, 4};
const std::array<double, 4> arr = {2.0, 4.0, 6.0, 8.0};
const std::string str = "abcd";

BOOST_AUTO_TEST_CASE(test_access) {
  int i = 0;
  for (const auto &[a, b, c] : Acts::zip(vec, arr, str)) {
    BOOST_CHECK_EQUAL(a, vec[i]);
    BOOST_CHECK_EQUAL(b, arr[i]);
    BOOST_CHECK_EQUAL(c, str[i]);
    ++i;
  }
}

BOOST_AUTO_TEST_CASE(test_mutation) {
  std::vector<int> vec2 = vec;
  std::array<double, 4> arr2 = arr;
  std::string str2 = str;

  for (auto [a, b, c] : Acts::zip(vec2, arr2, str2)) {
    a *= 2;
    b *= 2;
    c = 'e';
  }

  for (int i = 0; i < 4; ++i) {
    BOOST_CHECK_EQUAL(vec2[i], 2 * vec[i]);
    BOOST_CHECK_EQUAL(arr2[i], 2 * arr[i]);
  }

  BOOST_CHECK_EQUAL(str2, "eeee");
}
