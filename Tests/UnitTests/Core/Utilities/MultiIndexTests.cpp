// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/MultiIndex.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include <boost/mpl/list.hpp>

// 32bit split into a three level hierarchy.
using Index32 = Acts::MultiIndex<std::uint32_t, 16, 8, 8>;
// 64bit split into a four level hierarchy
using Index64 = Acts::MultiIndex<std::uint64_t, 13, 17, 21, 13>;
using Indices = boost::mpl::list<Index32, Index64>;

BOOST_AUTO_TEST_CASE_TEMPLATE(triviality, T, Indices) {
  // verify that the MultiIndex is a trivial type
  // this seems to be compiler-dependent; disable for now
  BOOST_CHECK(std::is_standard_layout_v<T>);
  BOOST_CHECK(std::is_trivially_default_constructible_v<T>);
  BOOST_CHECK(std::is_trivially_copy_constructible_v<T>);
  BOOST_CHECK(std::is_trivially_move_constructible_v<T>);
  BOOST_CHECK((std::is_trivially_assignable_v<T, T>));
  BOOST_CHECK((std::is_trivially_copy_assignable_v<T>));
  BOOST_CHECK((std::is_trivially_move_assignable_v<T>));
}

BOOST_AUTO_TEST_CASE(index32_construct) {
  // construct from encoded value
  {
    Index32 idx(0xabcd2400u);
    BOOST_CHECK_EQUAL(idx.value(), 0xabcd2400u);
    BOOST_CHECK_EQUAL(idx.level(0), 0xabcdu);
    BOOST_CHECK_EQUAL(idx.level(1), 0x24u);
    BOOST_CHECK_EQUAL(idx.level(2), 0x00u);
  }
  // assign from encoded value
  {
    Index32 idx = 0xabcd2400u;
    BOOST_CHECK_EQUAL(idx.value(), 0xabcd2400u);
    BOOST_CHECK_EQUAL(idx.level(0), 0xabcdu);
    BOOST_CHECK_EQUAL(idx.level(1), 0x24u);
    BOOST_CHECK_EQUAL(idx.level(2), 0x00u);
  }
}

BOOST_AUTO_TEST_CASE(index32_set) {
  Index32 idx = Index32::Zeros();
  // set a specific level within limits
  idx.set(0, 24u);
  BOOST_CHECK_EQUAL(idx.value(), 0x00180000u);
  BOOST_CHECK_EQUAL(idx.level(0), 24u);
  BOOST_CHECK_EQUAL(idx.level(1), 0u);
  BOOST_CHECK_EQUAL(idx.level(2), 0u);
  // set a specific level outside the valid range, should be truncated
  idx.set(2, 0xfff);
  BOOST_CHECK_EQUAL(idx.value(), 0x001800ffu);
  BOOST_CHECK_EQUAL(idx.level(0), 24u);
  BOOST_CHECK_EQUAL(idx.level(1), 0u);
  BOOST_CHECK_EQUAL(idx.level(2), 255u);
}

BOOST_AUTO_TEST_CASE(index32_set_overflow) {
  // maximum values for each level
  Index32::Value maxValues[] = {
      (1u << 16u) - 1u,
      (1u << 8u) - 1u,
      (1u << 8u) - 1u,
  };
  // check that values above max are truncated
  std::size_t lvl = 0;
  for (auto maxValue : maxValues) {
    BOOST_CHECK_EQUAL(Index32::Zeros().set(lvl, maxValue + 1),
                      Index32::Zeros().set(lvl, 0u));
    lvl += 1;
  }
}

BOOST_AUTO_TEST_CASE(index64_set_overflow) {
  // maximum values for each level
  Index32::Value maxValues[] = {
      (1u << 13u) - 1u,
      (1u << 17u) - 1u,
      (1u << 21u) - 1u,
      (1u << 13u) - 1u,
  };
  // check that values above max are truncated
  std::size_t lvl = 0;
  for (auto maxValue : maxValues) {
    BOOST_CHECK_EQUAL(Index64::Zeros().set(lvl, maxValue + 1),
                      Index64::Zeros().set(lvl, 0u));
    lvl += 1;
  }
}

BOOST_AUTO_TEST_CASE(index32_encode) {
  // all three levels set
  auto idx3 = Index32::Encode(13u, 14u, 15u);
  BOOST_CHECK_EQUAL(idx3.level(0), 13u);
  BOOST_CHECK_EQUAL(idx3.level(1), 14u);
  BOOST_CHECK_EQUAL(idx3.level(2), 15u);
  // first two levels set
  auto idx2 = Index32::Encode(13u, 14u);
  BOOST_CHECK_EQUAL(idx2.level(0), 13u);
  BOOST_CHECK_EQUAL(idx2.level(1), 14u);
  BOOST_CHECK_EQUAL(idx2.level(2), 0u);
  // only the first level set
  auto idx1 = Index32::Encode(13u);
  BOOST_CHECK_EQUAL(idx1.level(0), 13u);
  BOOST_CHECK_EQUAL(idx1.level(1), 0u);
  BOOST_CHECK_EQUAL(idx1.level(2), 0u);
  // nothing set
  auto idx0 = Index32::Encode();
  BOOST_CHECK_EQUAL(idx0, Index32::Zeros());
  BOOST_CHECK_EQUAL(idx0.value(), 0u);
  BOOST_CHECK_EQUAL(idx0.level(0), 0u);
  BOOST_CHECK_EQUAL(idx0.level(1), 0u);
  BOOST_CHECK_EQUAL(idx0.level(2), 0u);
}

BOOST_AUTO_TEST_CASE(index64_encode) {
  // all four levels set
  auto idx4 = Index64::Encode(23u, 14u, 15u, 17u);
  BOOST_CHECK_EQUAL(idx4.level(0), 23u);
  BOOST_CHECK_EQUAL(idx4.level(1), 14u);
  BOOST_CHECK_EQUAL(idx4.level(2), 15u);
  BOOST_CHECK_EQUAL(idx4.level(3), 17u);
  // first three levels set
  auto idx3 = Index64::Encode(23u, 14u, 15u);
  BOOST_CHECK_EQUAL(idx3.level(0), 23u);
  BOOST_CHECK_EQUAL(idx3.level(1), 14u);
  BOOST_CHECK_EQUAL(idx3.level(2), 15u);
  BOOST_CHECK_EQUAL(idx3.level(3), 0u);
  // first two levels set
  auto idx2 = Index64::Encode(23u, 14u);
  BOOST_CHECK_EQUAL(idx2.level(0), 23u);
  BOOST_CHECK_EQUAL(idx2.level(1), 14u);
  BOOST_CHECK_EQUAL(idx2.level(2), 0u);
  BOOST_CHECK_EQUAL(idx2.level(3), 0u);
  // only the first level set
  auto idx1 = Index64::Encode(23u);
  BOOST_CHECK_EQUAL(idx1.level(0), 23u);
  BOOST_CHECK_EQUAL(idx1.level(1), 0u);
  BOOST_CHECK_EQUAL(idx1.level(2), 0u);
  BOOST_CHECK_EQUAL(idx1.level(3), 0u);
  // nothing set
  auto idx0 = Index64::Encode();
  BOOST_CHECK_EQUAL(idx0, Index64::Zeros());
  BOOST_CHECK_EQUAL(idx0.value(), 0u);
  BOOST_CHECK_EQUAL(idx0.level(0), 0u);
  BOOST_CHECK_EQUAL(idx0.level(1), 0u);
  BOOST_CHECK_EQUAL(idx0.level(2), 0u);
  BOOST_CHECK_EQUAL(idx0.level(3), 0u);
}

BOOST_AUTO_TEST_CASE(uint32_sibling) {
  auto idx = Index32::Encode(1u, 12u, 123u);
  BOOST_CHECK_EQUAL(idx.makeNextSibling(0), Index32::Encode(2u));
  BOOST_CHECK_EQUAL(idx.makeNextSibling(1), Index32::Encode(1u, 13u));
  BOOST_CHECK_EQUAL(idx.makeNextSibling(2), Index32::Encode(1u, 12u, 124u));
}

BOOST_AUTO_TEST_CASE(uint64_sibling) {
  auto idx = Index64::Encode(1u, 12u, 123u, 1234u);
  BOOST_CHECK_EQUAL(idx.makeNextSibling(0), Index64::Encode(2u));
  BOOST_CHECK_EQUAL(idx.makeNextSibling(1), Index64::Encode(1u, 13u));
  BOOST_CHECK_EQUAL(idx.makeNextSibling(2), Index64::Encode(1u, 12u, 124u));
  BOOST_CHECK_EQUAL(idx.makeNextSibling(3),
                    Index64::Encode(1u, 12u, 123u, 1235u));
}

BOOST_AUTO_TEST_CASE(uint32_descendant) {
  auto idx = Index32::Encode(1u, 12u, 123u);
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(0), Index32::Encode(1u, 255u, 255u));
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(1), Index32::Encode(1u, 12u, 255u));
  // last index has no descendant and stays the same
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(2), Index32::Encode(1u, 12u, 123u));
}

BOOST_AUTO_TEST_CASE(uint64_descendant) {
  auto idx = Index64::Encode(1u, 12u, 123u, 1234u);
  // from Index64 definition above
  auto max1 = (1u << 17) - 1u;
  auto max2 = (1u << 21) - 1u;
  auto max3 = (1u << 13) - 1u;
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(0),
                    Index64::Encode(1u, max1, max2, max3));
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(1),
                    Index64::Encode(1u, 12u, max2, max3));
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(2),
                    Index64::Encode(1u, 12u, 123u, max3));
  // last index has no descendant and stays the same
  BOOST_CHECK_EQUAL(idx.makeLastDescendant(3),
                    Index64::Encode(1u, 12u, 123u, 1234u));
}

BOOST_AUTO_TEST_CASE(index32_as_key) {
  std::unordered_set<Index32> set;

  set.emplace(Index32::Encode(1u, 2u, 4u));
  set.emplace(Index32::Encode(1u, 3u, 4u));
  set.emplace(Index32::Encode(2u));

  BOOST_CHECK(!set.count(Index32(0u)));
  BOOST_CHECK(!set.count(Index32(std::numeric_limits<std::uint32_t>::max())));
  BOOST_CHECK_EQUAL(set.size(), 3);
  // automatically converts encoded value to MultiIndex
  BOOST_CHECK(set.count(0x00010204u));
  BOOST_CHECK(set.count(0x00010304u));
  BOOST_CHECK(set.count(Index32::Encode(2u)));
}

BOOST_AUTO_TEST_CASE(index64_as_key) {
  std::unordered_set<Index64> set;

  set.emplace(Index64::Encode(1u, 1u, 1u));
  set.emplace(Index64::Encode(2u));
  // same as above
  set.emplace(Index64::Encode(2u, 0u, 0u, 0u));
  set.emplace(Index64::Encode(2u, 1u));

  BOOST_CHECK(!set.count(Index64(0u)));
  BOOST_CHECK(!set.count(Index64(std::numeric_limits<std::uint64_t>::max())));
  BOOST_CHECK_EQUAL(set.size(), 3);
}
