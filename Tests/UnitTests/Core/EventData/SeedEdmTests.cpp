// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/EventData/Seed.hpp"

#include <vector>

namespace Acts::Test {

struct SpacePoint {};

BOOST_AUTO_TEST_CASE(seed_edm_constructors) {
  std::array<Acts::Test::SpacePoint, 5> storage{};
  Acts::Seed<Acts::Test::SpacePoint, 5> seed(storage[0], storage[1], storage[2],
                                             storage[3], storage[4]);
  const std::array<const Acts::Test::SpacePoint*, 5>& sps = seed.sp();
  for (std::size_t i(0ul); i < 5ul; ++i) {
    BOOST_CHECK_NE(sps[i], nullptr);
    BOOST_CHECK_EQUAL(sps[i], &storage[i]);
  }

  // Copy 1
  Acts::Seed<Acts::Test::SpacePoint, 5> seedCopy1(seed);
  const std::array<const Acts::Test::SpacePoint*, 5>& spsCopy1 = seedCopy1.sp();
  for (std::size_t i(0ul); i < 5ul; ++i) {
    BOOST_CHECK_NE(spsCopy1[i], nullptr);
    BOOST_CHECK_EQUAL(spsCopy1[i], sps[i]);
  }

  // Copy 2
  Acts::Seed<Acts::Test::SpacePoint, 5> seedCopy2{seed};
  const std::array<const Acts::Test::SpacePoint*, 5>& spsCopy2 = seedCopy2.sp();
  for (std::size_t i(0ul); i < 5ul; ++i) {
    BOOST_CHECK_NE(spsCopy2[i], nullptr);
    BOOST_CHECK_EQUAL(spsCopy2[i], sps[i]);
  }

  // Collection
  std::vector<Acts::Seed<Acts::Test::SpacePoint, 5>> seeds{seed};
  // Copy 1
  std::vector<Acts::Seed<Acts::Test::SpacePoint, 5>> seedsCopy1(seeds);
  BOOST_CHECK_EQUAL(seedsCopy1.size(), seeds.size());
  // Copy 2
  std::vector<Acts::Seed<Acts::Test::SpacePoint, 5>> seedsCopy2{seeds};
  BOOST_CHECK_EQUAL(seedsCopy2.size(), seeds.size());
}

BOOST_AUTO_TEST_CASE(seed_edm_default) {
  std::array<Acts::Test::SpacePoint, 3> storage{};
  Acts::Seed<Acts::Test::SpacePoint> seed(storage[0], storage[1], storage[2]);
  const std::array<const Acts::Test::SpacePoint*, 3>& sps = seed.sp();
  for (std::size_t i(0ul); i < 3ul; ++i) {
    BOOST_CHECK_NE(sps[i], nullptr);
    BOOST_CHECK_EQUAL(sps[i], &storage[i]);
  }

  seed.setVertexZ(-1.2f);
  BOOST_CHECK_EQUAL(seed.z(), -1.2f);

  seed.setQuality(345.23f);
  BOOST_CHECK_EQUAL(seed.seedQuality(), 345.23f);
}

BOOST_AUTO_TEST_CASE(seed_edm_3d) {
  std::array<Acts::Test::SpacePoint, 3> storage{};
  Acts::Seed<Acts::Test::SpacePoint, 3> seed(storage[0], storage[1],
                                             storage[2]);
  const std::array<const Acts::Test::SpacePoint*, 3>& sps = seed.sp();
  for (std::size_t i(0ul); i < 3ul; ++i) {
    BOOST_CHECK_NE(sps[i], nullptr);
    BOOST_CHECK_EQUAL(sps[i], &storage[i]);
  }

  seed.setVertexZ(-1.2f);
  BOOST_CHECK_EQUAL(seed.z(), -1.2f);

  seed.setQuality(345.23f);
  BOOST_CHECK_EQUAL(seed.seedQuality(), 345.23f);
}

BOOST_AUTO_TEST_CASE(seed_edm_4d) {
  std::array<Acts::Test::SpacePoint, 4> storage{};
  Acts::Seed<Acts::Test::SpacePoint, 4> seed(storage[0], storage[1], storage[2],
                                             storage[3]);
  const std::array<const Acts::Test::SpacePoint*, 4>& sps = seed.sp();
  for (std::size_t i(0ul); i < 4ul; ++i) {
    BOOST_CHECK_NE(sps[i], nullptr);
    BOOST_CHECK_EQUAL(sps[i], &storage[i]);
  }

  seed.setVertexZ(-1.2f);
  BOOST_CHECK_EQUAL(seed.z(), -1.2f);

  seed.setQuality(345.23f);
  BOOST_CHECK_EQUAL(seed.seedQuality(), 345.23f);
}

}  // namespace Acts::Test
