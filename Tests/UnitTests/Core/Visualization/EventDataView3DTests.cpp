// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Visualization/ObjVisualization3D.hpp"
#include "Acts/Visualization/PlyVisualization3D.hpp"

#include <algorithm>
#include <iostream>

#include "EventDataView3DBase.hpp"
#include "Visualization3DTester.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(Visualization)

BOOST_AUTO_TEST_CASE(BoundTrackParametersVisualizationObj) {
  ObjVisualization3D obj;
  auto objTest = EventDataView3DTest::testBoundTrackParameters(obj);
  auto objErrors = testObjString(objTest);
  BOOST_CHECK(objErrors.empty());
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }
  BOOST_CHECK_EQUAL(std::count(objTest.begin(), objTest.end(), '\n'), 4924);
}

BOOST_AUTO_TEST_CASE(BoundTrackParametersVisualizationPly) {
  PlyVisualization3D ply;
  auto plyTest = EventDataView3DTest::testBoundTrackParameters(ply);
  auto plyErrors = testPlyString(plyTest);
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }
  BOOST_CHECK_EQUAL(std::count(plyTest.begin(), plyTest.end(), '\n'), 3143);
}

BOOST_AUTO_TEST_CASE(MeasurementVisualizationObj) {
  ObjVisualization3D obj;
  auto objTest = EventDataView3DTest::testMeasurement(obj);
  auto objErrors = testObjString(objTest);
  BOOST_CHECK(objErrors.empty());
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }
  BOOST_CHECK_EQUAL(std::count(objTest.begin(), objTest.end(), '\n'), 520);
}

BOOST_AUTO_TEST_CASE(MeasurementVisualizationPly) {
  PlyVisualization3D ply;
  auto plyTest = EventDataView3DTest::testMeasurement(ply);
  auto plyErrors = testPlyString(plyTest);
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }
  BOOST_CHECK_EQUAL(std::count(plyTest.begin(), plyTest.end(), '\n'), 536);
}

BOOST_AUTO_TEST_CASE(MeasurementVisualizationFaultySettings) {
  ObjVisualization3D obj;

  double localErrorScale = 0.;
  BOOST_CHECK_THROW(EventDataView3DTest::testMeasurement(obj, localErrorScale),
                    std::invalid_argument);

  localErrorScale = -1.;
  BOOST_CHECK_THROW(EventDataView3DTest::testMeasurement(obj, localErrorScale),
                    std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(MultiTrajectoryVisualizationObj) {
  ObjVisualization3D obj;
  auto objTest = EventDataView3DTest::testMultiTrajectory(obj);
  auto objErrors = testObjString(objTest);
  BOOST_CHECK(objErrors.empty());
  for (const auto& objerr : objErrors) {
    std::cout << objerr << std::endl;
  }
  BOOST_CHECK_EQUAL(std::count(objTest.begin(), objTest.end(), '\n'), 103796);
}

BOOST_AUTO_TEST_CASE(MultiTrajectoryVisualizationPly) {
  PlyVisualization3D ply;
  auto plyTest = EventDataView3DTest::testMultiTrajectory(ply);
  auto plyErrors = testPlyString(plyTest);
  BOOST_CHECK(plyErrors.empty());
  for (const auto& plyerr : plyErrors) {
    std::cout << plyerr << std::endl;
  }
  BOOST_CHECK_EQUAL(std::count(plyTest.begin(), plyTest.end(), '\n'), 66091);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
