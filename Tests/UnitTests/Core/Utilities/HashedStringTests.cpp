// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/HashedString.hpp"

#include <string>
#include <string_view>

using namespace Acts::HashedStringLiteral;

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(string_hashes) {
  // compile time checks
  static_assert(hashString("abc") == 440920331, "Invalid with func");
  static_assert("abc"_hash == 440920331, "Invalid with literal");
  static_assert("abc"_hash == hashString("abc"), "Invalid");

  // runtime checks
  BOOST_CHECK_EQUAL(hashString("abc"), 440920331);
  BOOST_CHECK_EQUAL("abc"_hash, 440920331);

  std::string s = "abc";
  BOOST_CHECK_EQUAL(hashString(s), 440920331);
  constexpr std::string_view sv{"abc"};
  BOOST_CHECK_EQUAL(hashString(sv), 440920331);
  static_assert(hashString(sv) == 440920331, "Invalid");
}

}  // namespace Acts::Test
