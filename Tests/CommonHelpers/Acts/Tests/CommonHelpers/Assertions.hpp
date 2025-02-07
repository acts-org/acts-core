// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <vector>

#define CHECK_NE_COLLECTIONS(col1, col2)                                 \
  do {                                                                   \
    BOOST_CHECK_EQUAL(col1.size(), col2.size());                         \
    std::vector<bool> result;                                            \
    for (std::size_t i = 0; i < col1.size(); i++) {                      \
      result.push_back(col1[i] == col2[i]);                              \
    }                                                                    \
    BOOST_CHECK(!std::ranges::all_of(result, [](bool r) { return r; })); \
  } while (0)
