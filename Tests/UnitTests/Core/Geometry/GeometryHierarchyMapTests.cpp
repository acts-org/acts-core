// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryHierarchyMap.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"

#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

using Acts::GeometryIdentifier;

// helper function to create geometry ids
GeometryIdentifier makeId(int volume = 0, int layer = 0, int sensitive = 0) {
  return GeometryIdentifier().setVolume(volume).setLayer(layer).setSensitive(
      sensitive);
}

// example value type stored in the geometry hierarchy map
struct Thing {
  double value = 1.0;
};

using Container = Acts::GeometryHierarchyMap<Thing>;

}  // namespace

// check that an entry exists for the query and compare the id
#define CHECK_ENTRY(container, query, compare)          \
  do {                                                  \
    auto ret = container.find(query);                   \
    BOOST_CHECK_NE(ret, container.end());               \
    if (ret != container.end()) {                       \
      auto idx = std::distance(container.begin(), ret); \
      BOOST_CHECK_EQUAL(container.idAt(idx), compare);  \
    }                                                   \
  } while (false)

BOOST_TEST_DONT_PRINT_LOG_VALUE(Container::Iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(Thing)

BOOST_AUTO_TEST_SUITE(GeometryHierarchyMap)

BOOST_AUTO_TEST_CASE(ConstructDefault) {
  Container c;
  BOOST_CHECK_EQUAL(c.begin(), c.end());
  BOOST_CHECK(c.empty());
  BOOST_CHECK_EQUAL(c.size(), 0u);
}

BOOST_AUTO_TEST_CASE(ConstructNonUnique) {
  std::vector<std::pair<GeometryIdentifier, Thing>> entries = {
      {makeId(2, 4, 6), {1.0}},
      {makeId(3, 5), {1.0}},
      {makeId(3), {1.0}},
      // duplicate identifier
      {makeId(2, 4, 6), {2.0}},
  };
  BOOST_CHECK_THROW(Container(std::move(entries)), std::invalid_argument);

  std::vector<std::pair<GeometryIdentifier, Thing>> defaults = {
      {makeId(), {1.0}},
      // duplicate global default
      {makeId(), {2.0}},
  };
  BOOST_CHECK_THROW(Container(std::move(defaults)), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ConstructInitializerList) {
  Container c = {
      {makeId(0, 1, 2), {1.0}},
      {makeId(3, 4), {3.0}},
      {makeId(2), {2.0}},
      {makeId(), {23.0}},
  };
  BOOST_CHECK_EQUAL(std::next(c.begin(), 4), c.end());
  BOOST_CHECK(!c.empty());
  BOOST_CHECK_EQUAL(c.size(), 4u);
  // only test that all elements are there; failure test are below
  CHECK_ENTRY(c, makeId(0, 1, 2), makeId(0, 1, 2));
  CHECK_ENTRY(c, makeId(2), makeId(2));
  CHECK_ENTRY(c, makeId(3, 4), makeId(3, 4));
}

BOOST_AUTO_TEST_CASE(IndexBasedAccess) {
  Container c({
      {makeId(1, 2, 3), {2.0}},
      {makeId(3, 4, 5), {2.5}},
      {makeId(3, 5), {3.0}},
      {makeId(4, 5, 7), {4.0}},
  });

  BOOST_CHECK(!c.empty());
  BOOST_CHECK_EQUAL(c.size(), 4u);
  // this tests just that the index-based access works
  // NOTE order is undefined and should not be tested
  for (auto i = c.size(); 0 < i--;) {
    // just check that the id is valid
    BOOST_CHECK_NE(c.idAt(i), GeometryIdentifier());
    // check that something is actually stored by comparing with the default
    BOOST_CHECK_NE(c.valueAt(i).value, Thing().value);
  }
  // test that invalid inputs actually fail
  BOOST_CHECK_THROW(c.idAt(c.size()), std::out_of_range);
  BOOST_CHECK_THROW(c.valueAt(c.size()), std::out_of_range);
}

BOOST_AUTO_TEST_CASE(Find) {
  Container c = {
      // entry for volume 2, layer 4, sensitive 6
      {makeId(2, 4, 6), {-23.0}},
      // entry for volume 2, layer 8
      {makeId(2, 8), {5.0}},
      // entry for the whole volume 2
      {makeId(2), {1.0}},
      // entry for volume 12, layer 16
      // NOTE no entry for the volume as a whole
      {makeId(12, 16), {-1.0}},
  };

  // basic checks
  BOOST_CHECK_EQUAL(std::next(c.begin(), 4u), c.end());
  BOOST_CHECK(!c.empty());
  BOOST_CHECK_EQUAL(c.size(), 4u);

  // find existing sensitive
  CHECK_ENTRY(c, makeId(2, 4, 6), makeId(2, 4, 6));
  // find existing layer
  CHECK_ENTRY(c, makeId(2, 8), makeId(2, 8));
  // find existing volume
  CHECK_ENTRY(c, makeId(2), makeId(2));
  // find existing layer
  CHECK_ENTRY(c, makeId(12, 16), makeId(12, 16));

  // find non-existing sensitive, which has a set volume
  CHECK_ENTRY(c, makeId(2, 4, 7), makeId(2));
  // find non-existing layer id, which has a set volume
  CHECK_ENTRY(c, makeId(2, 13), makeId(2));
  // find non-existing sensitive id, which has a set layer
  CHECK_ENTRY(c, makeId(2, 8, 13), makeId(2, 8));
  // find non-existing sensitive, which has a set layer
  CHECK_ENTRY(c, makeId(12, 16, 20), makeId(12, 16));

  // find non-existing sensitive, which has no higher hierarchy set
  BOOST_CHECK_EQUAL(c.find(makeId(3, 5, 7)), c.end());
  // find non-existing layer, which has no higher hierarchy set
  BOOST_CHECK_EQUAL(c.find(makeId(3, 5)), c.end());
  // find non-existing volume
  BOOST_CHECK_EQUAL(c.find(makeId(3)), c.end());
  // find non-existing volume, which has only lower hierarchy elements
  BOOST_CHECK_EQUAL(c.find(makeId(12)), c.end());
}

BOOST_AUTO_TEST_CASE(FindWithGlobalDefault) {
  Container c = {
      // global default entry
      {makeId(), {1.0}},
      // entry for volume 2, layer 3
      {makeId(2, 3), {2.0}},
      // entry for volume 4
      {makeId(4), {4.0}},
  };

  // basic checks
  BOOST_CHECK_EQUAL(std::next(c.begin(), 3u), c.end());
  BOOST_CHECK(!c.empty());
  BOOST_CHECK_EQUAL(c.size(), 3u);

  // find existing entries
  CHECK_ENTRY(c, makeId(), makeId());
  CHECK_ENTRY(c, makeId(2, 3), makeId(2, 3));
  CHECK_ENTRY(c, makeId(4), makeId(4));

  // find missing sensitive w/ set layer
  CHECK_ENTRY(c, makeId(2, 3, 4), makeId(2, 3));
  // find missing layer w/ set volume
  CHECK_ENTRY(c, makeId(4, 5), makeId(4));

  // find missing sensitive w/o set volume/layer -> global default
  CHECK_ENTRY(c, makeId(2, 4, 5), makeId());
  // find missing layer w/o set volume -> global default
  CHECK_ENTRY(c, makeId(2, 4), makeId());
  // find missing volume
  CHECK_ENTRY(c, makeId(5), makeId());
}

BOOST_AUTO_TEST_SUITE_END()
