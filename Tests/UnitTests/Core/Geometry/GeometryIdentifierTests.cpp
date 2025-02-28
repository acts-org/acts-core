// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Geometry/GeometryIdentifier.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(GeometryIdentifier_construct_default) {
  GeometryIdentifier id;
  BOOST_CHECK_EQUAL(id.volume(), 0u);
  BOOST_CHECK_EQUAL(id.boundary(), 0u);
  BOOST_CHECK_EQUAL(id.layer(), 0u);
  BOOST_CHECK_EQUAL(id.approach(), 0u);
  BOOST_CHECK_EQUAL(id.sensitive(), 0u);
}

BOOST_AUTO_TEST_CASE(GeometryIdentifier_construct_encoded) {
  // not sure if it is a good idea to test for the encoding since it should be
  // an implementation detail. only the resulting functionality is relevant.
  GeometryIdentifier id = 0xa0b00c00d00affe0u;
  BOOST_CHECK_EQUAL(id.volume(), 0xa0u);
  BOOST_CHECK_EQUAL(id.boundary(), 0xb0u);
  BOOST_CHECK_EQUAL(id.layer(), 0x0c0u);
  BOOST_CHECK_EQUAL(id.approach(), 0x0du);
  BOOST_CHECK_EQUAL(id.sensitive(), 0x00affu);
  BOOST_CHECK_EQUAL(id.extra(), 0xe0u);
}

BOOST_AUTO_TEST_CASE(GeometryIdentifier_max_values) {
  // compute maximum value for each component
  constexpr GeometryIdentifier::Value volumeMax = (1u << 8) - 1;
  constexpr GeometryIdentifier::Value boundaryMax = (1u << 8) - 1;
  constexpr GeometryIdentifier::Value layerMax = (1u << 12) - 1;
  constexpr GeometryIdentifier::Value approachMax = (1u << 8) - 1;
  constexpr GeometryIdentifier::Value sensitiveMax = (1u << 20) - 1;
  constexpr GeometryIdentifier::Value extraMax = (1u << 8) - 1;
  // reference values non-zero values everywhere
  constexpr GeometryIdentifier ref = 0xdeadaffe01234567;
  // values above the maximum are truncated
  // max+1 has all available bits zeroed
  BOOST_CHECK_THROW(
      static_cast<void>(GeometryIdentifier(ref).withVolume(volumeMax + 1)),
      std::invalid_argument);
  BOOST_CHECK_THROW(
      static_cast<void>(GeometryIdentifier(ref).withBoundary(boundaryMax + 1)),
      std::invalid_argument);
  BOOST_CHECK_THROW(
      static_cast<void>(GeometryIdentifier(ref).withLayer(layerMax + 1)),
      std::invalid_argument);
  BOOST_CHECK_THROW(
      static_cast<void>(GeometryIdentifier(ref).withApproach(approachMax + 1)),
      std::invalid_argument);
  BOOST_CHECK_THROW(static_cast<void>(GeometryIdentifier(ref).withSensitive(
                        sensitiveMax + 1)),
                    std::invalid_argument);
  BOOST_CHECK_THROW(
      static_cast<void>(GeometryIdentifier(ref).withExtra(extraMax + 1)),
      std::invalid_argument);

  BOOST_CHECK_EQUAL(GeometryIdentifier::getMaxVolume(), 255);
  BOOST_CHECK_EQUAL(GeometryIdentifier::getMaxBoundary(), 255);
  BOOST_CHECK_EQUAL(GeometryIdentifier::getMaxLayer(), 4095);
  BOOST_CHECK_EQUAL(GeometryIdentifier::getMaxApproach(), 255);
  BOOST_CHECK_EQUAL(GeometryIdentifier::getMaxSensitive(), 1048575);
  BOOST_CHECK_EQUAL(GeometryIdentifier::getMaxExtra(), 255);
}

BOOST_AUTO_TEST_CASE(GeometryIdentifier_order) {
  auto vol1 = GeometryIdentifier()
                  .withVolume(1u)
                  .withLayer(14u)
                  .withSensitive(5u)
                  .withExtra(42u);
  auto vol2 = GeometryIdentifier()
                  .withVolume(2u)
                  .withLayer(13u)
                  .withSensitive(3u)
                  .withExtra(43u);

  // order uses volume first even if other components are larger
  BOOST_CHECK_LT(vol1, vol2);
  BOOST_CHECK_LT(vol1.withBoundary(64u), vol2);
  BOOST_CHECK_LT(vol1.withLayer(64u), vol2);
  BOOST_CHECK_LT(vol1.withApproach(64u), vol2);
  BOOST_CHECK_LT(vol1.withSensitive(64u), vol2);
  BOOST_CHECK_LT(vol1.withSensitive(64u), vol2);
  BOOST_CHECK_LT(vol2, GeometryIdentifier(vol1).withVolume(3u));
  // other components are hierarchical
  BOOST_CHECK_LT(vol1.withVolume(1u).withBoundary(2u),
                 vol1.withVolume(2u).withBoundary(1u));
  BOOST_CHECK_LT(vol1.withBoundary(1u).withLayer(2u),
                 vol1.withBoundary(2u).withLayer(1u));
  BOOST_CHECK_LT(vol1.withLayer(1u).withApproach(2u),
                 vol1.withLayer(2u).withApproach(1u));
  BOOST_CHECK_LT(vol1.withApproach(1u).withSensitive(2u),
                 vol1.withApproach(2u).withSensitive(1u));
  BOOST_CHECK_LT(vol1.withSensitive(1u).withExtra(2u),
                 vol1.withSensitive(2u).withExtra(1u));
}

}  // namespace Acts::Test
