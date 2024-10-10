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
  Acts::GeometryIdentifier geoId;
  geoId.setSensitive(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});
  BOOST_CHECK(aGroup.has(geoId));

  Acts::GeometryIdentifier badId;
  badId.setSensitive(2);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Single_layer_Test) {
  Acts::GeometryIdentifier geoId;
  geoId.setLayer(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});
  BOOST_CHECK(aGroup.has(geoId));

  Acts::GeometryIdentifier badId;
  badId.setLayer(2);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Single_volume_Test) {
  Acts::GeometryIdentifier geoId;
  geoId.setVolume(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});
  BOOST_CHECK(aGroup.has(geoId));

  Acts::GeometryIdentifier badId;
  badId.setVolume(2);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Hierarchy_test) {
  Acts::GeometryIdentifier geoId;
  geoId.setVolume(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});

  Acts::GeometryIdentifier geoId2;
  geoId2.setVolume(1);
  geoId2.setLayer(2);
  BOOST_CHECK(aGroup.has(geoId2));

  Acts::GeometryIdentifier geoId3;
  geoId3.setVolume(1);
  geoId3.setLayer(3);
  BOOST_CHECK(aGroup.has(geoId3));
}

BOOST_AUTO_TEST_CASE(Hierarchy_test_2) {
  Acts::GeometryIdentifier geoId;
  geoId.setVolume(1);
  geoId.setLayer(1);
  ActsExamples::AlignmentGroup aGroup("test", {geoId});

  Acts::GeometryIdentifier badId;
  badId.setVolume(1);
  BOOST_CHECK(!aGroup.has(badId));
}

BOOST_AUTO_TEST_CASE(Multiple_test) {
  Acts::GeometryIdentifier geoId1;
  geoId1.setVolume(1);
  geoId1.setLayer(1);
  Acts::GeometryIdentifier geoId2;
  geoId2.setVolume(2);
  geoId2.setLayer(2);
  Acts::GeometryIdentifier geoId3;
  geoId3.setVolume(3);
  geoId3.setLayer(3);
  Acts::GeometryIdentifier geoId4;
  geoId4.setVolume(4);
  geoId4.setLayer(4);

  ActsExamples::AlignmentGroup aGroup("test", {geoId1, geoId2, geoId3});

  BOOST_CHECK(aGroup.has(geoId1));
  BOOST_CHECK(aGroup.has(geoId2));
  BOOST_CHECK(aGroup.has(geoId3));
  BOOST_CHECK(!aGroup.has(geoId4));
}
