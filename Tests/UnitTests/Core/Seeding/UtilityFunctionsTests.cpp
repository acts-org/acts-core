// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Seeding/detail/UtilityFunctions.hpp"

#include <iterator>
#include <list>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(pushBackOrInsertAtEnd_vector) {
  std::vector<std::size_t> coll;
  Acts::detail::pushBackOrInsertAtEnd(coll, 2ul);
  BOOST_CHECK(coll.size() == 1ul);
  Acts::detail::pushBackOrInsertAtEnd(coll, 5ul);
  BOOST_CHECK(coll.size() == 2ul);
  std::size_t val = 1ul;
  Acts::detail::pushBackOrInsertAtEnd(coll, val);
  BOOST_CHECK(coll.size() == 3ul);

  BOOST_CHECK_EQUAL(coll[0], 2ul);
  BOOST_CHECK_EQUAL(coll[1], 5ul);
  BOOST_CHECK_EQUAL(coll[2], 1ul);
}

BOOST_AUTO_TEST_CASE(pushBackOrInsertAtEnd_list) {
  std::list<std::size_t> coll;
  Acts::detail::pushBackOrInsertAtEnd(coll, 2ul);
  BOOST_CHECK(coll.size() == 1ul);
  Acts::detail::pushBackOrInsertAtEnd(coll, 5ul);
  BOOST_CHECK(coll.size() == 2ul);
  std::size_t val = 1ul;
  Acts::detail::pushBackOrInsertAtEnd(coll, val);
  BOOST_CHECK(coll.size() == 3ul);

  BOOST_CHECK_EQUAL(coll.front(), 2ul);
  coll.pop_front();
  BOOST_CHECK_EQUAL(coll.front(), 5ul);
  coll.pop_front();
  BOOST_CHECK_EQUAL(coll.front(), 1ul);
  coll.pop_front();
}

BOOST_AUTO_TEST_CASE(pushBackOrInsertAtEnd_set) {
  std::set<std::size_t> coll;
  Acts::detail::pushBackOrInsertAtEnd(coll, 2ul);
  BOOST_CHECK(coll.size() == 1ul);
  Acts::detail::pushBackOrInsertAtEnd(coll, 5ul);
  BOOST_CHECK(coll.size() == 2ul);
  std::size_t val = 1ul;
  Acts::detail::pushBackOrInsertAtEnd(coll, val);
  BOOST_CHECK(coll.size() == 3ul);

  BOOST_CHECK(coll.contains(2ul));
  BOOST_CHECK(coll.contains(5ul));
  BOOST_CHECK(coll.contains(1ul));
}

BOOST_AUTO_TEST_CASE(pushBackOrInsertAtEnd_unordered_set) {
  std::unordered_set<std::size_t> coll;
  Acts::detail::pushBackOrInsertAtEnd(coll, 2ul);
  BOOST_CHECK(coll.size() == 1ul);
  Acts::detail::pushBackOrInsertAtEnd(coll, 5ul);
  BOOST_CHECK(coll.size() == 2ul);
  std::size_t val = 1ul;
  Acts::detail::pushBackOrInsertAtEnd(coll, val);
  BOOST_CHECK(coll.size() == 3ul);

  BOOST_CHECK(coll.contains(2ul));
  BOOST_CHECK(coll.contains(5ul));
  BOOST_CHECK(coll.contains(1ul));
}

}  // namespace Acts::Test
