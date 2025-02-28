// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include <ActsExamples/Alignment/AlignmentAlgorithm.hpp>

BOOST_AUTO_TEST_CASE(Single_sensitive_Test) {
  auto geoId = Acts::GeometryIdentifier().withSensitive(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});
  BOOST_CHECK(aGroup.has(geoId));

  auto badId = Acts::GeometryIdentifier().withSensitive(2);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Single_layer_Test) {
  auto geoId = Acts::GeometryIdentifier().withLayer(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});
  BOOST_CHECK(aGroup.has(geoId));

  auto badId = Acts::GeometryIdentifier().withLayer(2);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Single_volume_Test) {
  auto geoId = Acts::GeometryIdentifier().withVolume(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});
  BOOST_CHECK(aGroup.has(geoId));

  auto badId = Acts::GeometryIdentifier().withVolume(2);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Hierarchy_test) {
  auto geoId = Acts::GeometryIdentifier().withVolume(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});

  auto geoId2 = Acts::GeometryIdentifier().withVolume(1).withLayer(2);
  BOOST_CHECK(aGroup.has(geoId2));

  auto geoId3 = Acts::GeometryIdentifier().withVolume(1).withLayer(3);
  BOOST_CHECK(aGroup.has(geoId3));
}

BOOST_AUTO_TEST_CASE(Hierarchy_test_2) {
  auto geoId = Acts::GeometryIdentifier().withVolume(1).withLayer(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});

  auto badId = Acts::GeometryIdentifier().withVolume(1);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Multiple_test) {
  auto geoId1 = Acts::GeometryIdentifier().withVolume(1).withLayer(1);
  auto geoId2 = Acts::GeometryIdentifier().withVolume(2).withLayer(2);
  auto geoId3 = Acts::GeometryIdentifier().withVolume(3).withLayer(3);
  auto geoId4 = Acts::GeometryIdentifier().withVolume(4).withLayer(4);

  ActsExamples::AlignmentGroup aGroup("test", {geoId1, geoId2, geoId3});

  BOOST_CHECK(aGroup.has(geoId1));
  BOOST_CHECK(aGroup.has(geoId2));
  BOOST_CHECK(aGroup.has(geoId3));
  BOOST_CHECK(!aGroup.has(geoId4));
}
